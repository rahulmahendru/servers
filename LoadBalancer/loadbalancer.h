#include <err.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <stdatomic.h>
#include <signal.h>

#include "queue.h"

#define SUCCESS 1
#define FAILURE -1

#define INVALID -1

#define WAIT_TIME 3
#define BUFFER_SIZE 4096

typedef struct server_info_t {
    uint16_t port;
    bool alive;
    atomic_int total_requests;
    atomic_int total_errors;
} ServerInfo;

typedef struct servers_t {
    uint16_t port;
    ssize_t num_threads;
    ssize_t req_delay;
    int num_servers;
    atomic_int requests;
    ServerInfo* servers;
    pthread_mutex_t mut;
    pthread_cond_t cond;
    pthread_mutex_t health_mut;
    pthread_cond_t health_cond;
} Servers;