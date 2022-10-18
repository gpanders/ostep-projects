#include <assert.h>

#include "io_helper.h"
#include "queue.h"

void
queue_init(struct queue *q, size_t size)
{
    q->data = malloc_or_die(size * sizeof(int));
    q->read = 0;
    q->write = 0;
    q->capacity = size;
    q->len = 0;
}

void
queue_push(struct queue *q, int v)
{
    assert(q->len < q->capacity);
    q->data[q->write] = v;
    q->write = (q->write + 1) % q->capacity;
    q->len++;
}

int
queue_pop(struct queue *q)
{
    assert(q->len > 0);
    int v = q->data[q->read];
    q->read = (q->read + 1) % q->capacity;
    q->len--;
    return v;
}

bool
queue_empty(struct queue *q)
{
    return q->len == 0;
}
