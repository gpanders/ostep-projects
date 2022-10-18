#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int
wunzip(FILE *fp)
{
    int rc = 0;

    if (fseek(fp, 0, SEEK_END)) {
        perror("fseek");
        return 1;
    }

    size_t fsize = ftell(fp);
    rewind(fp);

    char *p = mmap(NULL, fsize, PROT_READ, MAP_SHARED, fileno(fp), 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    char *buf = NULL;
    size_t bufsz = 0;
    uint32_t run_length = 0;
    char c = 0;
    for (size_t i = 0; i < fsize; i += sizeof(run_length) + sizeof(c)) {
        memcpy(&run_length, &p[i], sizeof(run_length));
        memcpy(&c, &p[i + sizeof(run_length)], sizeof(c));

        if (!buf) {
            buf = calloc(run_length, sizeof(*buf));
            if (!buf) {
                perror("calloc");
                rc = 1;
                goto out;
            }
            bufsz = run_length;
        }

        if (bufsz < run_length) {
            char *tmp = realloc(buf, 2 * run_length);
            if (!tmp) {
                perror("realloc");
                rc = 1;
                goto out;
            }
            buf = tmp;
            bufsz = 2 * run_length;
        }

        memset(buf, c, run_length);
        fwrite(buf, sizeof(c), run_length, stdout);
    }

out:
    if (buf) {
        free(buf);
    }

    munmap(p, fsize);

    return rc;
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("wunzip: file1 [file2 ...]\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            printf("wunzip: cannot open file\n");
            return 1;
        }

        int rc = wunzip(fp);
        fclose(fp);
        if (rc) {
            return rc;
        }
    }

    return 0;
}
