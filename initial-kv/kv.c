#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUCKETS 64

struct kv {
    int k;
    char *v;
};

struct entry {
    struct kv kv;
    struct entry *next;
};

struct db {
    struct entry *entries[BUCKETS];
};

struct db_iter {
    struct db *db;
    size_t n;
    struct entry *ent;
};

struct cmd {
    enum {
        CMD_PUT,
        CMD_GET,
        CMD_CLR,
        CMD_ALL,
        CMD_DEL,
    } type;
    struct kv kv;
};

int
kv_parse(struct kv *kv, char *str)
{
    char *s = str;
    char *token = NULL;
    int i = 0;

    int k;
    char *v;

    while ((token = strsep(&s, ",")) != NULL) {
        switch (i) {
        case 0:
            k = atoi(token);
            break;
        case 1:
            v = token;
            break;
        default:
            fprintf(stderr, "Invalid key-value pair: %s\n", str);
            return 1;
        }

        i++;
    }

    if (i < 2) {
        fprintf(stderr, "Invalid key-value pair: %s\n", str);
        return 1;
    }

    kv->k = k;
    kv->v = v;

    return 0;
}

void
db_insert(struct db *db, int key, char *val)
{
    size_t n = key % BUCKETS;
    struct entry *cur = db->entries[n];
    struct entry *parent = NULL;
    while (cur) {
        if (cur->kv.k == key) {
            break;
        }
        parent = cur;
        cur = cur->next;
    }

    if (!cur) {
        cur = malloc(sizeof(*cur));
        if (!cur) {
            perror("malloc");
            exit(1);
        }

        cur->kv.k = key;
        cur->next = NULL;

        if (parent) {
            parent->next = cur;
        } else {
            db->entries[n] = cur;
        }
    } else {
        free(cur->kv.v);

    }

    cur->kv.v = strdup(val);
}

void
db_delete(struct db *db, int key)
{
    size_t n = key % BUCKETS;
    struct entry *cur = db->entries[n];
    struct entry *parent = NULL;
    while (cur) {
        if (cur->kv.k == key) {
            break;
        }
        parent = cur;
        cur = cur->next;
    }

    if (!cur) {
        return;
    }

    if (parent) {
        parent->next = cur->next;
    } else {
        db->entries[n] = cur->next;
    }

    free(cur->kv.v);
    free(cur);
}

const char *
db_get(struct db *db, int key)
{
    size_t n = key % BUCKETS;
    struct entry *cur = db->entries[n];
    while (cur) {
        if (cur->kv.k == key) {
            break;
        }
        cur = cur->next;
    }

    if (cur) {
        return cur->kv.v;
    }

    return NULL;
}

void
db_read(struct db *db, const char *filename)
{
    memset(db, 0, sizeof(*db));

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        if (errno == ENOENT) {
            // File does not exist yet, simply return
            return;
        }

        perror("fopen");
        exit(1);
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t sz = 0;
    struct kv kv;
    while ((sz = getline(&line, &linecap, fp)) > 0) {
        if (line[sz - 1] == '\n') {
            line[sz - 1] = '\0';
        }

        if (kv_parse(&kv, line)) {
            exit(1);
        }

        db_insert(db, kv.k, kv.v);
    }

    if (sz < 0 && ferror(fp)) {
        perror("getline");
        exit(1);
    }

    if (line) {
        free(line);
    }

    if (fclose(fp)) {
        perror("fclose");
    }
}

void
db_write(struct db *db, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    for (size_t n = 0; n < BUCKETS; n++) {
        struct entry *cur = db->entries[n];
        while (cur) {
            fprintf(fp, "%d,%s\n", cur->kv.k, cur->kv.v);
            cur = cur->next;
        }
    }

    if (fclose(fp)) {
        perror("fclose");
    }
}

void
db_clear(struct db *db)
{
    for (size_t n = 0; n < BUCKETS; n++) {
        struct entry *cur = db->entries[n];
        struct entry *tmp = NULL;
        while (cur) {
            tmp = cur->next;
            free(cur->kv.v);
            free(cur);
            cur = tmp;
        }

        db->entries[n] = NULL;
    }
}

void
db_iter_init(struct db *db, struct db_iter *iter)
{
    iter->db = db;
    iter->n = 0;
    iter->ent = db->entries[0];
}

struct kv *
db_iter_next(struct db_iter *iter)
{
    struct kv *kv = NULL;
    struct entry *cur = iter->ent;
    while (!cur) {
        if (iter->n >= BUCKETS - 1) {
            return NULL;
        }

        cur = iter->db->entries[++(iter->n)];
    }

    kv = &cur->kv;
    iter->ent = cur->next;
    return kv;
}

int
cmd_parse(struct cmd *cmd, char *str)
{
    char *s = str;
    char *token = NULL;
    char *type = NULL;
    char *k = NULL;
    char *v = NULL;
    int i = 0;

    while ((token = strsep(&s, ","))  != NULL) {
        switch (i) {
        case 0:
            type = token;
            break;
        case 1:
            k = token;
            break;
        case 2:
            v = token;
            break;
        default:
            fprintf(stderr, "Invalid command: '%s'\n", str);
            return 1;
        }
        i++;
    }

    if (!strcmp(type, "p")) {
        cmd->type = CMD_PUT;
    } else if (!strcmp(type, "g")) {
        cmd->type = CMD_GET;
    } else if (!strcmp(type, "d")) {
        cmd->type = CMD_DEL;
    } else if (!strcmp(type, "c")) {
        cmd->type = CMD_CLR;
    } else if (!strcmp(type, "a")) {
        cmd->type = CMD_ALL;
    } else {
        fprintf(stderr, "Invalid command: '%s'\n", str);
        return 1;
    }

    if (cmd->type == CMD_PUT || cmd->type == CMD_GET || cmd->type == CMD_DEL) {
        if (k) {
            cmd->kv.k = atoi(k);
        } else {
            fprintf(stderr, "Command is missing required key: '%s'\n", str);
            return 1;
        }
    } else if (k) {
        fprintf(stderr, "Command does not accept a key: '%s'\n", str);
        return 1;
    }

    if (cmd->type == CMD_PUT) {
        if (v) {
            cmd->kv.v = v;
        } else {
            fprintf(stderr, "Command is missing required value: '%s'\n", str);
            return 1;
        }
    } else if (v) {
        fprintf(stderr, "Command does not accept a value: '%s'\n", str);
        return 1;
    }

    return 0;
}

void
cmd_print(struct cmd *cmd)
{
    switch (cmd->type) {
    case CMD_PUT:
        printf("Put: %d = %s\n", cmd->kv.k, cmd->kv.v);
        break;
    case CMD_GET:
        printf("Get: %d\n", cmd->kv.k);
        break;
    case CMD_DEL:
        printf("Delete: %d\n", cmd->kv.k);
        break;
    case CMD_CLR:
        printf("Clear\n");
        break;
    case CMD_ALL:
        printf("All\n");
        break;
    }
}

void
cmd_handle(struct cmd *cmd, struct db *db)
{
    struct kv *kv = &cmd->kv;
    switch (cmd->type) {
    case CMD_PUT: {
        db_insert(db, kv->k, kv->v);
        break;
    }
    case CMD_GET: {
        const char *v = db_get(db, kv->k);
        if (v) {
            printf("%d,%s\n", kv->k, v);
        } else {
            printf("%d not found\n", kv->k);
        }
        break;
    }
    case CMD_CLR:
        db_clear(db);
        break;
    case CMD_ALL: {
        struct db_iter iter;
        db_iter_init(db, &iter);
        while ((kv = db_iter_next(&iter)) != NULL) {
            printf("%d,%s\n", kv->k, kv->v);
        }
        break;
    }
    case CMD_DEL:
        db_delete(db, kv->k);
        break;
    }
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    struct db db;
    db_read(&db, "database.txt");

    int rc = 0;
    for (int i = 1; i < argc; i++) {
        char *s = strdup(argv[i]);
        struct cmd cmd;
        int rc = cmd_parse(&cmd, argv[i]);
        if (rc) {
            free(s);
            rc = 1;
            break;
        }

        cmd_handle(&cmd, &db);
        free(s);
    }

    if (rc == 0) {
        db_write(&db, "database.txt");
    }

    db_clear(&db);

    return rc;
}
