#include "server.h"

// Global initializations
static int offset = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static const char* log_termin = "========\n";
static atomic_int requests = 0;
static atomic_int errors = 0;

// Thread handling function
void* thread_function(void* thread) {
    Thread_t *worker = (Thread_t*)thread;
    while (1) {
        pthread_mutex_lock(&lock);
        if((worker->client_sockd = dequeue()) == INVALID) {
            pthread_cond_wait(&cond, &lock);
            worker->client_sockd = dequeue();
        }
        pthread_mutex_unlock(&lock);
        if(worker->client_sockd != INVALID) {
            handle_connection(worker);
        }
        close(worker->client_sockd);
        worker->client_sockd = INT_MIN;
    } 
    return NULL;
}

// To handle the connection
void handle_connection(Thread_t *worker) {
    memset(&worker->message, 0, sizeof(worker->message));

    // Read response
    read_http_response(worker->client_sockd, &worker->message);
    if (worker->message.status_code != OK) {
        send_error_response(worker->client_sockd, &worker->message);
        if (strcmp(worker->logfile, "") != 0) {
            log_request(worker);
        }
        requests++;
        errors++;
        return;
    }

    // For healthcheck
    char* filename = worker->message.filename;
    ++filename;
    if (strcmp(filename, "healthcheck") == 0) {
        healthcheck(worker);
        if (strcmp(worker->logfile, "") != 0) {
            log_request(worker);
        }
        requests++;
        return;
    }

    // Process Request
    process_request(worker->client_sockd, &worker->message);
    requests++;
    if (worker->message.status_code != OK && worker->message.status_code != CREATED) {
        errors++;
        send_error_response(worker->client_sockd, &worker->message);
        if (strcmp(worker->logfile, "") != 0) {
            log_request(worker);
        }
        return;
    }

    // To log request into logfile
    if (strcmp(worker->logfile, "") != 0) {
        log_request(worker);
    }
}

void log_request(Thread_t *worker) {
    struct httpObject* message = &worker->message;
    char buff[BUFFER_SIZE + 1];
    memset(&buff, '\0', BUFFER_SIZE + 1);
    int fd = open(worker->logfile, O_WRONLY);
    
    // For errors
    if (message->status_code != OK && message->status_code != CREATED) {

        // Create response
        int bytes_a = snprintf(buff, BUFFER_SIZE, "FAIL: %s %s %s --- response %d\n", message->method, message->filename, message->httpversion, message->status_code);
        strcat(buff, log_termin);
        int bytes = bytes_a + strlen(log_termin);

        // Lock for offset allocation
        pthread_mutex_lock(&lock);
        message->offset = offset;
        offset += bytes;
        pthread_mutex_unlock(&lock);

        // Write to log file
        if (pwrite(fd, buff, bytes , message->offset) < 0) {
            perror("Logging");
            return;
        }
        close(fd);
        return;
    }

    // For HEAD request
    else if (strcmp(message->method, "HEAD") == 0) {
        int bytes_a = snprintf(buff, BUFFER_SIZE, "%s %s length %ld\n", message->method, message->filename, message->content_length);
        strcat(buff, log_termin);
        int bytes = bytes_a + strlen(log_termin);

        pthread_mutex_lock(&lock);
        message->offset = offset;
        offset += bytes;
        pthread_mutex_unlock(&lock);

        if (pwrite(fd, buff, bytes , message->offset) < 0) {
            perror("Logging");
            return;
        }
        close(fd);
        return;
    }

    // For GET and PUT
    else {
        int bytes_a = snprintf(buff, BUFFER_SIZE, "%s %s length %ld\n", message->method, message->filename, message->content_length);
        int bytes = bytes_a + strlen(log_termin);
        int temp = message->content_length;

        // Get bytes according to content length
        while (temp > 0) {
            if (temp > 20) {
                bytes += LOG_BUFF_SIZE;
                temp -= 20;
            }
            else {
                bytes += 9 + (temp * 3);
                break;
            }
        }

        pthread_mutex_lock(&lock);
        message->offset = offset;
        offset += bytes;
        pthread_mutex_unlock(&lock);

        if (pwrite(fd, buff, bytes_a , message->offset) < 0 ) {
            perror("Logging");
            return;
        }
        message->offset += bytes_a;

        char* filename = message->filename;
        ++filename;
        char logbuffer[LOG_BUFF_SIZE + 1];

        if (strcmp(filename, "healthcheck") == 0) {
            int log_offset = 0;
            int logged = snprintf(logbuffer, LOG_BUFF_SIZE ,"00000000");
            log_offset += logged;
            int count = 0;
            int bytes_r = strlen(message->smallbuffer);
            while (count < bytes_r) {
                logged = snprintf(logbuffer+log_offset, LOG_BUFF_SIZE , " %02x", (message->smallbuffer[count++]) & 0xff);
                log_offset += logged;
            }
            logged = snprintf(logbuffer + log_offset, LOG_BUFF_SIZE , "\n");
            log_offset += logged;
            if (pwrite(fd, logbuffer, log_offset , message->offset) < 0 ) {
                perror("Logging");
                return;
            }
            message->offset += log_offset;
        }
        else {
            // Open file to read data from to log
            int8_t file = open(filename, O_RDONLY);
            if (file < 0) {
                return;
            }

            ssize_t total = 0;
            int line = 0;
            char smallbuff[21];
            while(total < message->content_length){
                memset(&smallbuff, '\0', 21);
                memset(&logbuffer, '\0', LOG_BUFF_SIZE+1);
                int log_offset = 0;
                int count = 0;

                ssize_t bytes_r = read(file, smallbuff, 20);
                if (bytes_r < 0) {
                    perror("read");
                    return;
                }
                int logged = snprintf(logbuffer, LOG_BUFF_SIZE , "%08d", line);
                log_offset += logged;
                
                while (count < bytes_r) {
                    // if (smallbuff[count] == '\0') break;
                    logged = snprintf(logbuffer+log_offset, LOG_BUFF_SIZE , " %02x", (smallbuff[count++]) & 0xff);
                    log_offset += logged;
                }
                logged = snprintf(logbuffer + log_offset, LOG_BUFF_SIZE , "\n");
                log_offset += logged;
                total += bytes_r;
                if (pwrite(fd, logbuffer, log_offset , message->offset) < 0) {
                    perror("logging");
                    return;
                }
                message->offset += log_offset;
                line += 20;
            }
            close(file);
        }
        if (pwrite(fd, log_termin, strlen(log_termin), message->offset) < 0) {
            perror("read");
            return;
        }
        close(fd);
        return;
    }
}

void healthcheck(Thread_t* worker) {
    const char* method = worker->message.method;
    if (strcmp(method, "GET") != 0) {
        dprintf(worker->client_sockd, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
        errors++;
        worker->message.status_code = FORBIDDEN;
    }
    else if (strcmp(worker->logfile, "") == 0 ) {
        dprintf(worker->client_sockd, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
        errors++;
        worker->message.status_code = NOT_FOUND;
    }
    else {
        memset(&worker->message.smallbuffer, '\0', sizeof(worker->message.smallbuffer));
        snprintf(worker->message.smallbuffer, 128, "%d\n%d", errors, requests);
        worker->message.content_length = strlen(worker->message.smallbuffer);
        dprintf(worker->client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n%s", worker->message.content_length, worker->message.smallbuffer);
    }
}

// Main Function
int main(int argc, char** argv) {

    // Check for invalid call to program
    if ((argc < 2) || (argc > 6)) {
        errno = EIO;
        perror("Input");
        return EXIT_FAILURE;
    }

    input_arg arg;
    memset(&arg, 0, sizeof(arg));
    arg.num_threads = 4;
    if ( parseArgs(argc, argv, &arg) != 0 ) {
        errno = EIO;
        perror("Input");
        return EXIT_FAILURE;
    }

    if (strcmp(arg.logfile, "") != 0) {
        if (create_logfile(arg.logfile) < 0) {
            perror("Logfile");
            return EXIT_FAILURE;
        }
    }

    // Create sockaddr_in with server information
    char* port = arg.port;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addrlen = sizeof(server_addr);

    //Create server socket
    int server_sockd = socket(AF_INET, SOCK_STREAM, 0);

    // Need to check if server_sockd < 0, meaning an error
    if (server_sockd < 0) {
        perror("socket");
    }

    // Configure server socket
    int enable = 1;

    // This allows you to avoid: 'Bind: Address Already in Use' error
    int ret = setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // Bind server address to socket that is open
    ret = bind(server_sockd, (struct sockaddr *) &server_addr, addrlen);

    // Check for bind error
    if (ret < 0) {
        perror("Bind");
        return EXIT_FAILURE;
    }

    // Listen for incoming connections
    ret = listen(server_sockd, SOMAXCONN);

    // Check for listen error
    if (ret < 0) {
        perror("Listen");
        return EXIT_FAILURE;
    }

    // Connecting with a client
    struct sockaddr client_addr;
    socklen_t client_addrlen;

    // Make the thread pool
    Thread_t t_pool[arg.num_threads];
    for (int idx = 0; idx < arg.num_threads; ++idx) {
        t_pool[idx].client_sockd = INT_MIN;
        memset(&t_pool[idx].message, 0, sizeof(t_pool[idx].message));
        strcpy(t_pool[idx].logfile, arg.logfile);
        pthread_create(&t_pool[idx].worker, NULL, thread_function, &t_pool[idx]);
    }

    // Loop to accept client request
    while (true) {
	    memset(&client_addrlen, 0, sizeof(client_addrlen));
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);
        if (client_sockd < 0) {
            perror("Accept");
            return EXIT_FAILURE;
        }
        pthread_mutex_lock(&lock);
        enqueue(client_sockd);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }

    // Close server
    close (server_sockd);

    return EXIT_SUCCESS;
}
