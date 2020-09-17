#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h> // write
#include <string.h> // memset
#include <stdlib.h> // atoi
#include <stdbool.h> // true, false
#include <errno.h>
#include <pthread.h>
#include <limits.h> // INT_MIN
#include <err.h>
#include <stdatomic.h>

#include "queue.h"

#define BUFFER_SIZE 4096
#define LOG_BUFF_SIZE 69

// Response Codes
#define OK           200        
#define CREATED      201
#define BAD_REQUEST  400
#define FORBIDDEN    403
#define NOT_FOUND    404
#define INT_SRVR_ERR 500

#define INVALID -1

typedef struct argObject {
    char port[5];
    ssize_t num_threads;
    char logfile[29];
} input_arg;

struct httpObject {
    char method[128];         // PUT, HEAD, GET
    char filename[128];      // what is the file we are worried about
    char httpversion[128];    // HTTP/1.1
    ssize_t content_length; // example: 13
    int status_code;
    ssize_t buffer_bytes;
    uint8_t buffer[BUFFER_SIZE + 1];
    char smallbuffer[129];
    int offset;             
};

// Thread object
typedef struct threadargs_t {
    int client_sockd;
    struct httpObject message;
    pthread_t worker;
    char logfile[29];
} Thread_t;


// To parse the program arguments
int parseArgs(int argc, char* argv[], struct argObject* arg);

// To create a log file
int create_logfile(const char* logfile);

// To handle the connection
void handle_connection(Thread_t *worker);

// to read the http request header
void read_http_response(ssize_t client_sockd, struct httpObject* message);

// to perform the requested function
void process_request(ssize_t client_sockd, struct httpObject* message);

// Check if method name is valid
bool check_method(const char *method);

// Check if filename is valid
bool check_filename(const char *filename);

// check if http version is valid
bool check_httpversion(const char* httpversion);

// run the get method
void server_get(ssize_t client_sockd, struct httpObject* message);

// run the head method
void server_head(ssize_t client_sockd, struct httpObject* message);

// run the put method
void server_put(ssize_t client_sockd, struct httpObject* message);

// send error response in case of an error
void send_error_response(ssize_t client_sockd, struct httpObject* message);

// To perform the log operations
void log_request(Thread_t *worker);

// To perform the health check
void healthcheck(Thread_t* worker);

#endif
