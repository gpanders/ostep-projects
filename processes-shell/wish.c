#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>
#include <unistd.h>

#define MAXARGS 256
#define MAXPATHS 64

#define MAX(a, b) (a > b ? a : b)

static char prompt[] = "wish> ";
static char error_message[] = "An error has occurred\n";

static void
error()
{
	write(STDERR_FILENO, error_message, sizeof(error_message) - 1);
}

struct wish {
	bool done;
	uint8_t status;
	size_t nargs;
	size_t npaths;
	size_t njobs;
	char *args[MAXARGS];
	char *paths[MAXPATHS];
	int redirfd;
};

static void
wish_init(struct wish *wish)
{
	wish->done = false;
	wish->status = 0;
	wish->redirfd = 0;
	wish->nargs = 0;
	wish->njobs = 0;

	memset(wish->args, 0, sizeof(wish->args));
	memset(wish->paths, 0, sizeof(wish->paths));

	wish->paths[0] = strdup("/bin");
	wish->npaths = 1;
}

static void
wish_free(struct wish *wish)
{
	for (size_t i = 0; i < wish->npaths; i++) {
		free(wish->paths[i]);
	}

	wish->npaths = 0;
}

static void
wish_err(struct wish *wish)
{
	wish->done = true;
	error();
}

static bool
handle_builtin(struct wish *wish)
{
	assert(wish->nargs > 0);

	size_t nargs = wish->nargs;
	const char *cmd = wish->args[0];
	char **args = &wish->args[1];

	if (!strcmp(cmd, "exit")) {
		int status = 0;
		if (nargs > 1) {
			for (size_t i = 0; i < strlen(args[0]); i++) {
				if (!isdigit(args[0][i])) {
					error();
					status = 1;
					break;
				}

				status = 10 * status + (args[0][i] - '0');
			}

			if (status > 255) {
				status = 255;
			}
		}

		wish->status = (uint8_t)status;
		wish->done = true;
		return true;
	} else if (!strcmp(cmd, "cd")) {
		if (nargs != 2 || chdir(args[0]) != 0) {
			wish_err(wish);
		}

		return true;
	} else if (!strcmp(cmd, "path")) {
		size_t npaths = nargs - 1;
		for (size_t i = 0; i < MAX(npaths, wish->npaths); i++) {
			if (i < wish->npaths) {
				free(wish->paths[i]);
			}

			if (i < npaths) {
				wish->paths[i] = strdup(args[i]);
			}
		}

		wish->npaths = npaths;

		return true;
	}

	return false;
}

static void
exec(struct wish *wish)
{
	assert(wish->nargs > 0);

	size_t nargs = wish->nargs;
	wish->args[nargs] = NULL;

	if (wish->redirfd) {
		if (dup2(wish->redirfd, STDOUT_FILENO) == -1) {
			return wish_err(wish);
		}

		if (dup2(wish->redirfd, STDERR_FILENO) == -1) {
			return wish_err(wish);
		}
	}

	const char *cmd = wish->args[0];
	if (cmd[0] == '/') {
		execv(cmd, wish->args);
		return;
	}

	char fullcmd[PATH_MAX];
	for (size_t i = 0; i < wish->npaths; i++) {
		snprintf(fullcmd, PATH_MAX, "%s/%s", wish->paths[i], cmd);
		execv(fullcmd, wish->args);
		if (errno != ENOENT) {
			break;
		}
	}
}

static void
parse_command(struct wish *wish, char *cmd)
{
	char *s = cmd;
	char *tok;
	size_t nargs = 0;

	while ((tok = strsep(&s, " \t")) != NULL) {
		if (*tok == '\0') {
			continue;
		}

		wish->args[nargs++] = tok;
	}

	wish->nargs = nargs;
}

static char *
parse_redirect(char *str)
{
	char *s = str;
	while (*s != '\0' && isspace(*s)) {
		s++;
	}

	char *fname = strsep(&s, " \t");
	if (!fname || *fname == '\0') {
		return NULL;
	}

	// There should be no other whitespace characters
	while (s && *s != '\0' && isspace(*s)) {
		s++;
	}

	if (s && *s != '\0') {
		return NULL;
	}

	return fname;
}

static void
do_job(struct wish *wish, char *str)
{
	char *s = str;
	char *cmd = NULL;
	char *redir = NULL;

	// Skip leading whitespace
	while (*s != '\0' && isspace(*s)) {
		s++;
	}

	if (*s == '\0') {
		// Empty command
		return;
	}

	cmd = strsep(&s, ">");
	if (!cmd) {
		return wish_err(wish);
	}

	parse_command(wish, cmd);

	if (wish->nargs == 0) {
		return wish_err(wish);
	}

	if (s) {
		redir = parse_redirect(s);
		if (!redir) {
			return wish_err(wish);
		}
	}

	if (handle_builtin(wish)) {
		return;
	}

	if (redir) {
		int fd = open(redir, O_CREAT|O_TRUNC|O_WRONLY, 0666);
		if (fd < 0) {
			return wish_err(wish);
		}

		wish->redirfd = fd;
	}

	pid_t pid = fork();
	if (pid == 0) {
		setpgid(0, getpid());
		exec(wish);

		// If we make it here, execv failed
		return wish_err(wish);
	} else {
		wish->njobs++;
		if (wish->redirfd) {
			close(wish->redirfd);
			wish->redirfd = 0;
		}
	}
}

static void
do_line(struct wish *wish, char *line)
{
	char *s = line;
	char *tok;

	while ((tok = strsep(&s, "&")) != NULL) {
		do_job(wish, tok);
	}

	while (wish->njobs) {
		wait(NULL);
		wish->njobs--;
	}
}

int
main(int argc, char *argv[])
{
	if (argc > 2) {
		error();
		return 1;
	}

	FILE *fp = stdin;
	bool batch = false;
	if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (!fp) {
			error();
			return 1;
		}
		batch = true;
	}

	if (!batch) {
		// Ignore SIGINT in interactive mode
		struct sigaction sa;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_IGN;
		if (sigaction(SIGINT, &sa, NULL)) {
			perror("sigaction");
			return 1;
		}
	}

	struct wish wish;
	wish_init(&wish);

	char *line = NULL;
	size_t linecap = 0;
	while (!wish.done) {
		if (!batch) {
			write(STDOUT_FILENO, prompt, sizeof(prompt) - 1);
		}

		ssize_t sz = getline(&line, &linecap, fp);
		if (sz < 0) {
			wish.done = true;
		} else if (sz > 0) {
			size_t n = strspn(line, " \t\n");
			if (n == (size_t)sz) {
				// Empty line
				continue;
			}

			char *str = line + n;
			str[(size_t)sz - n - 1] = '\0';
			do_line(&wish, str);
		}
	}

	wish_free(&wish);

	if (line) {
		free(line);
	}

	if (batch) {
		fclose(fp);
	}

	return 0;
}
