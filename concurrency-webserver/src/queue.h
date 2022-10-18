#include <stdbool.h>
#include <stddef.h>

struct queue {
    int *data;
    size_t read;
    size_t write;
    size_t capacity;
    size_t len;
};

void queue_init(struct queue *q, size_t size);
void queue_push(struct queue *q, int v);
int queue_pop(struct queue *q);
bool queue_empty(struct queue *q);
