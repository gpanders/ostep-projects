#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int
wcat(const char *filename)
{
	int rc = 0;
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		printf("wcat: cannot open file\n");
		return 1;
	}

	if (fseek(fp, 0, SEEK_END)) {
		perror("fseek");
		rc = 1;
		goto out;
	}

	long fsize = ftell(fp);
	rewind(fp);

	if (fsize == 0) {
		// Empty file, skip
		goto out;
	}

	char *p = mmap(NULL, fsize, PROT_READ, MAP_SHARED, fileno(fp), 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		rc = 1;
		goto out;
	}

	if (fwrite(p, sizeof(char), fsize, stdout) < fsize) {
		perror("fwrite");
		rc = 1;
		goto out;
	}

	if (munmap(p, fsize)) {
		perror("munmap");
		rc = 1;
		goto out;
	}
out:
	fclose(fp);

	return rc;
}

int
main(int argc, char *argv[])
{
	int rc = 0;
	if (argc < 2) {
		return 0;
	}

	for (int i = 1; i < argc; i++) {
		rc = wcat(argv[i]);
		if (rc) {
			return rc;
		}
	}

	return 0;
}
