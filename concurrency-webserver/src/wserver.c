#include <stdio.h>
#include <pthread.h>

#include "io_helper.h"
#include "request.h"
#include "queue.h"

char default_root[] = ".";

static pthread_mutex_t lock;
static pthread_cond_t cv;

void *
handle_request(void *arg)
{
    struct queue *q = arg;
    pthread_mutex_lock(&lock);
    while (queue_empty(q)) {
        pthread_cond_wait(&cv, &lock);
    }

    int conn_fd = queue_pop(q);
    pthread_mutex_unlock(&lock);

    request_handle(conn_fd);
    close_or_die(conn_fd);

    return NULL;
}

void
usage()
{
    fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffersize]\n");
}

//
// ./wserver [-d <basedir>] [-p <portnum>]
//
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
    int nthreads = 10;
    size_t queue_size = 10;

    struct queue q;

    while ((c = getopt(argc, argv, "d:p:t:b:h")) != -1)
        switch (c) {
        case 'd':
            root_dir = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 't':
            nthreads = atoi(optarg);
            break;
        case 'b':
            queue_size = atoi(optarg);
            break;
        case 'h':
            usage();
            exit(0);
        default:
            usage();
            exit(1);
        }

    // run out of this directory
    chdir_or_die(root_dir);

    queue_init(&q, queue_size);
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cv, NULL);

    pthread_t *threads = malloc_or_die(nthreads * sizeof(pthread_t));
    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&threads[i], NULL, handle_request, &q)) {
            perror("pthread_create");
            exit(1);
        }
    }

    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        int conn_fd = accept_or_die(listen_fd, (sockaddr_t *)&client_addr,
            (socklen_t *)&client_len);
        pthread_mutex_lock(&lock);
        queue_push(&q, conn_fd);
        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&lock);
    }
    return 0;
}
