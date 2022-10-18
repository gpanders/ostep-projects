#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int
wgrep(const char *pattern, FILE *fp)
{
	char *linep = NULL;
	size_t linecapp = 0;
	ssize_t sz = 0;
	while ((sz = getline(&linep, &linecapp, fp)) > 0) {
		if (strnstr(linep, pattern, sz) == NULL) {
			continue;
		}

		if (fwrite(linep, sizeof(char), sz, stdout) < (size_t)sz) {
			perror("fwrite");
			return 1;
		}
	}

	if (linep) {
		free(linep);
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("wgrep: searchterm [file ...]\n");
		return 1;
	}

	const char *pattern = argv[1];
	if (argc < 3) {
		return wgrep(pattern, stdin);
	}

	for (int i = 2; i < argc; i++) {
		FILE *fp = fopen(argv[i], "r");
		if (!fp) {
			printf("wgrep: cannot open file\n");
			return 1;
		}
		int rc = wgrep(pattern, fp);
		fclose(fp);
		if (rc) {
			return rc;
		}
	}

	return 0;
}
