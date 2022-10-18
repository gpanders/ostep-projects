#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

struct encoder {
    char c;
    uint32_t run_length;
    bool initialized;
};

void
encoder_init(struct encoder *enc)
{
    enc->c = 0;
    enc->run_length = 0;
    enc->initialized = false;
}

int
wzip(FILE *fp, struct encoder *enc)
{
    if (fseek(fp, 0, SEEK_END)) {
        perror("fseek");
        return 1;
    }

    size_t fsize = ftell(fp);
    rewind(fp);

    if (fsize == 0) {
        return 0;
    }

    char *p = mmap(NULL, fsize, PROT_READ, MAP_SHARED, fileno(fp), 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    size_t i = 0;

    if (!enc->initialized) {
        enc->c = p[0];
        enc->run_length = 1;
        enc->initialized = true;
        i++;
    }

    char c = enc->c;
    uint32_t run_length = enc->run_length;

    for (; i < fsize; i++) {
        if (p[i] == c) {
            run_length++;
        } else {
            fwrite(&run_length, sizeof(run_length), 1, stdout);
            fwrite(&c, sizeof(c), 1, stdout);
            c = p[i];
            run_length = 1;
        }
    }

    munmap(p, fsize);

    enc->c = c;
    enc->run_length = run_length;

    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    struct encoder enc;
    encoder_init(&enc);
    for (int i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            printf("wzip: cannot open file\n");
            return 1;
        }

        int rc = wzip(fp, &enc);
        fclose(fp);
        if (rc) {
            return rc;
        }
    }

    fwrite(&enc.run_length, sizeof(enc.run_length), 1, stdout);
    fwrite(&enc.c, sizeof(enc.c), 1, stdout);

    return 0;
}
