#include "server.h"

// Utility values
static const char termin[] = "\r\n\r\n";
static const ssize_t inc = 4;

// Parse Arguments
int parseArgs(int argc, char* argv[], struct argObject* arg) {
    int opt;
    int return_value = 0;
    while (1) {
        opt = getopt(argc, argv, "N:l:");

        // If all arguments are read or error occurs
        if (opt == -1) break;         

        // Store number of threads          
        if (opt == 'N') {
            arg->num_threads = atoi(optarg);
        }

        // Store log file
        else if (opt == 'l') {
            memcpy(arg->logfile, optarg, strlen(optarg));
        }

        // Return error for invalid flag
        else {
            return_value = -1;
        }
    }

    // Check for invalid number of arguments
    if ((argc==optind)||(argc-optind>1)) {
        return_value = -1;
        return return_value;
    }

    // Store port number and check if it is invalid
    if (optind != argc) {
        memcpy(arg->port, argv[optind], strlen(argv[optind]));
        int port = atoi(arg->port);
        if (port == 0) {
            return_value = -1;
        }
    }
    return return_value;
}

// To create the logfile
int create_logfile(const char* logfile) {
    mode_t mode = S_IRWXU;
    int fd = creat(logfile, mode);
    if (fd < 0) {
        return -1;
    }
    close(fd);
    return 0;
}


// Read the http response
void read_http_response(ssize_t client_sockd, struct httpObject* message) {
    char buff[BUFFER_SIZE + 1];
    char file[128];
    memset(&buff, '\0', BUFFER_SIZE + 1);
    memset(&message->filename, '\0', 128);
    memset(&message->method, '\0', 128);
    memset(&message->httpversion, '\0', 128);

    // Receive request
    ssize_t bytes = recv(client_sockd, buff, BUFFER_SIZE, 0);
    
    // Check for valid read
    if (bytes < 0) {
        message->status_code = BAD_REQUEST;
        return;
    }

    // Parse request
    sscanf(buff, "%s %s %s", message->method, file, message->httpversion);

    // Check if request is valid
    if (check_method(message->method) == false) {
        memcpy(message->filename, file , strlen(file));
        if (strcmp(message->filename, "/healthcheck") == 0) message->status_code = FORBIDDEN;
        else message->status_code = BAD_REQUEST;
        return;
    }

    if (check_filename(file) == false) {
        memset(&message->filename, '\0', 50);
        memcpy(message->filename, file , strlen(file));
        printf("%s\n", message->filename);
        message->status_code = BAD_REQUEST;
        return;
    }
    memcpy(message->filename, file , strlen(file));

    if (check_httpversion(message->httpversion) == false) {
        message->status_code = BAD_REQUEST;
        return;
    }
    
    // In case of PUT
    // get content-length
    char* token;
    int c_length;
            
    if ((token=strstr(buff, "Content-Length")) != NULL) {          
        sscanf(token, "Content-Length: %d", &c_length);
        if (c_length < 0) {
            message->status_code = BAD_REQUEST;
            return;
        }
        message->content_length = (ssize_t) c_length;
    }

    // Parse any data present after the header request
    if (message->content_length > 0) {
        memset(message->buffer, '\0', BUFFER_SIZE + 1);
        message->buffer_bytes = 0;
        char* data = strstr(buff, termin);
        data += inc;
        int size = strlen(data);
        if (size > 0) {
            memcpy(message->buffer, data, size+1);
            message->buffer_bytes = size;
        }
        dprintf(client_sockd, "HTTP/1.1 100-Continue\r\n\r\n");
    }

    message->status_code = OK;

    return;
}


// Process method and perform the function
void process_request(ssize_t client_sockd, struct httpObject* message) {

    if (strcmp(message->method, "GET") == 0) server_get(client_sockd, message);
    else if (strcmp(message->method, "HEAD") == 0) server_head(client_sockd, message);
    else if (strcmp(message->method, "PUT") == 0) server_put(client_sockd, message);

    return;
}


// Check if method is valid
bool check_method(const char *method) {
    if (strcmp(method, "GET") != 0 &&
        strcmp(method, "PUT") != 0 && 
        strcmp(method, "HEAD") != 0){
            return false;
    }
    return true;
}


// Check if the filename is valid
bool check_filename(const char *filename) {
    if(filename++[0] != '/') return false;        // For '/' at start of filename
    if (strlen(filename) > 27) return false;      // For characters to be at most 27

    // Check for illegal alphanumeric characters
    for (uint8_t index = 0; index < strlen(filename); ++index) {  
        int alpha  = (int) filename[index];
        if ((alpha < 48 || alpha > 57) &&
            (alpha < 65 || alpha > 90) &&
            (alpha < 97 || alpha > 122) &&
            (alpha != 45) && (alpha != 95)) {
                return false;
        }
    }
    return true;
}


// Check if httpversion is valid
bool check_httpversion(const char* httpversion) {
    if (strcmp(httpversion, "HTTP/1.1") != 0) {
        return false;
    }
    return true;
}


// Run the GET method
void server_get(ssize_t client_sockd, struct httpObject* message){
    memset(message->buffer, '\0', BUFFER_SIZE + 1);
    message->content_length = 0;

    char* filename = message->filename;
    ++filename;

    // Check if file exists
    int8_t status = access(filename, F_OK);
    if (status < 0 || errno == EACCES) {
        message->status_code = NOT_FOUND;
        return;
    }

    // check if it is a valid regular file (not directory) and has read permission
    struct stat file_des;
    int8_t fd = stat(filename, &file_des);
    if (fd < 0) {
        message->status_code = INT_SRVR_ERR;
        return;
    }
    if (S_ISREG(file_des.st_mode) == 0) {
        message->status_code = FORBIDDEN;
        return;
    }
    if ((file_des.st_mode & S_IRUSR) == 0) {
        message->status_code = FORBIDDEN;
        return;
    }

    // Open file and check for open errors
    int8_t file = open(filename, O_RDONLY);
    if (file < 0) {
        message->status_code = NOT_FOUND;
        return;
    }

    // Get content-length 
    message->content_length = (ssize_t) file_des.st_size;

    // send response
    dprintf(client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", message->content_length);

    // Read data from file and send to client
    ssize_t total = 0;
    while(total < message->content_length){
        ssize_t bytes = read(file, message->buffer, BUFFER_SIZE);
        if (bytes < 0) {
            message->status_code = INT_SRVR_ERR;
            return;
        }
        total += bytes;
        send(client_sockd, message->buffer, bytes, 0);
    }

    // Close file
    close(file);
    
    return;
}


// Run the HEAD method
void server_head(ssize_t client_sockd, struct httpObject* message){
    message->content_length = 0;

    char* filename = message->filename;
    ++filename;

    // Check if file exists
    int8_t status = access(filename, F_OK);
    if (status < 0 || errno == EACCES) {
        message->status_code = NOT_FOUND;
        return;
    }

    // check if it is a valid regular file and has read permissions
    struct stat file_des;
    int8_t fd = stat(filename, &file_des);
    if (fd < 0) {
        message->status_code = NOT_FOUND;
        return;
    }
    if (S_ISREG(file_des.st_mode) == 0) {
        message->status_code = FORBIDDEN;
        return;
    }
    if ((file_des.st_mode & S_IRUSR) == 0) {
        message->status_code = FORBIDDEN;
        return;
    }

    // Get content-length 
    message->content_length = (ssize_t) file_des.st_size;

    // Send response
    dprintf(client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", message->content_length);

    return;
}


// run the PUT method
void server_put(ssize_t client_sockd, struct httpObject* message){
    char* filename = message->filename;
    ++filename;

    // Check if file exists and is not a directory
    struct stat file_des;
    int8_t fd = stat(filename, &file_des);
    if (fd >= 0) {
        if (S_ISREG(file_des.st_mode) == 0) {
            message->status_code = FORBIDDEN;
            return;
        }
    }

    // Open file with Read/Write permissions
    mode_t mode = 0644;
    int8_t file = open(filename, O_RDWR | O_CREAT | O_TRUNC, mode);
    if (file < 0 || errno == EACCES) {
	    close(file);
        message->status_code = FORBIDDEN;
        return;
    }

    ssize_t bytes_r = 0;
    if (message->buffer_bytes > 0) {
        ssize_t bytes = write(file, message->buffer, message->buffer_bytes);
        if (bytes < 0){
            message->status_code = INT_SRVR_ERR;
            return;
        }
        bytes_r += bytes;
    }

    memset(message->buffer, '\0', BUFFER_SIZE + 1);

    while (bytes_r < message->content_length) {
        ssize_t bytes_recv = recv(client_sockd, message->buffer, BUFFER_SIZE, 0);
        if (bytes_recv < 0){
            message->status_code = INT_SRVR_ERR;
            return;
        }
        ssize_t bytes = write(file, message->buffer, bytes_recv);
        bytes_r += bytes;
    }
    dprintf(client_sockd, "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");

    close(file);

    return;
}


// Send error response
void send_error_response(ssize_t client_sockd, struct httpObject* message) {
    if (message->status_code == BAD_REQUEST)
        dprintf(client_sockd, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
    else if (message->status_code == FORBIDDEN)
        dprintf(client_sockd, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
    else if (message->status_code == NOT_FOUND)
        dprintf(client_sockd, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
    else if (message->status_code == INT_SRVR_ERR)
        dprintf(client_sockd, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
    return;
}
