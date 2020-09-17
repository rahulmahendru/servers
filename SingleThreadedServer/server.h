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

#define BUFFER_SIZE 4096

// Response Codes
#define OK           200        
#define CREATED      201
#define BAD_REQUEST  400
#define FORBIDDEN    403
#define NOT_FOUND    404
#define INT_SRVR_ERR 500

struct httpObject {
    char method[5];         // PUT, HEAD, GET
    char filename[29];      // what is the file we are worried about
    char httpversion[9];    // HTTP/1.1
    ssize_t content_length; // example: 13
    int status_code;
    ssize_t buffer_bytes;
    uint8_t buffer[BUFFER_SIZE + 1];
};

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

