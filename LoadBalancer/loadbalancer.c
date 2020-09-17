#include "loadbalancer.h"

Servers servers;
static const char* healthcheck = "GET /healthcheck HTTP/1.1\r\nContent-Length : 0\r\n\r\n";
static const char* error = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";

/*
 * client_connect takes a port number and establishes a connection as a client.
 * connectport: port number of server to connect to
 * returns: valid socket if successful, -1 otherwise
 */
int client_connect(uint16_t connectport) {
    int connfd;
    struct sockaddr_in servaddr;

    connfd=socket(AF_INET,SOCK_STREAM,0);
    if (connfd < 0)
        return -1;
    memset(&servaddr, 0, sizeof servaddr);

    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(connectport);

    /* For this assignment the IP address can be fixed */
    inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr));

    if(connect(connfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
        return -1;
    return connfd;
}

/*
 * server_listen takes a port number and creates a socket to listen on 
 * that port.
 * port: the port number to receive connections
 * returns: valid socket if successful, -1 otherwise
 */
int server_listen(int port) {
    int listenfd;
    int enable = 1;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
        return -1;
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        return -1;
    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof servaddr) < 0)
        return -1;
    if (listen(listenfd, 500) < 0)
        return -1;
    return listenfd;
}

/*
 * bridge_connections send up to 100 bytes from fromfd to tofd
 * fromfd, tofd: valid sockets
 * returns: number of bytes sent, 0 if connection closed, -1 on error
 */
int bridge_connections(int fromfd, int tofd) {
    char recvline[BUFFER_SIZE];
    int n = recv(fromfd, recvline, 100, 0);
    if (n < 0) {
        return -1;
    } else if (n == 0) {
        return 0;
    }
    recvline[n] = '\0';
    n = send(tofd, recvline, n, 0);
    if (n < 0) {
        return -1;
    } else if (n == 0) {
        return 0;
    }
    return n;
}

/*
 * bridge_loop forwards all messages between both sockets until the connection
 * is interrupted. It also prints a message if both channels are idle.
 * sockfd1, sockfd2: valid sockets
 */
void bridge_loop(int sockfd1, int sockfd2) {
    fd_set set;
    struct timeval timeout;

    int fromfd, tofd;
    while(1) {
        // set for select usage must be initialized before each select call
        // set manages which file descriptors are being watched
        FD_ZERO (&set);
        FD_SET (sockfd1, &set);
        FD_SET (sockfd2, &set);

        // same for timeout
        // max time waiting, 5 seconds, 0 microseconds
        timeout.tv_sec = WAIT_TIME;
        timeout.tv_usec = 0;

        // select return the number of file descriptors ready for reading in set
        switch (select(FD_SETSIZE, &set, NULL, NULL, &timeout)) {
            case -1:
                return;
            case 0:
                if (send(sockfd1, error, strlen(error), 0) < 0){
                    // warn("send_err\n");
                }
                return;
            default:
                if (FD_ISSET(sockfd1, &set)) {
                    fromfd = sockfd1;
                    tofd = sockfd2;
                } else if (FD_ISSET(sockfd2, &set)) {
                    fromfd = sockfd2;
                    tofd = sockfd1;
                } else {
                    return;
                }
        }
        if (bridge_connections(fromfd, tofd) <= 0)
            return;
    }
}

/*
 * Parse the health check response
*/
void parse_healthcheck(int idx, const char* buff) {
    int status;
    char temp[128];
    sscanf(buff, "HTTP/1.1 %d %s", &status, temp);
    if (status != 200) {
        servers.servers[idx].alive = false;
        return;
    }
    char* token;
    int errors = 0;
    int requests = 0;
    if ((token=strstr(buff, "\r\n\r\n")) != NULL) {          
        sscanf(token, "\r\n\r\n%d\n%d", &errors, &requests);
    }
    servers.servers[idx].total_requests = requests;
    servers.servers[idx].total_errors = errors;
    servers.servers[idx].alive = true;
    return;
}

/*
 * Perform health check on all the servers and update the server
*/
void do_health_check() {
    int connfd;
    char buff[BUFFER_SIZE];
    for (int idx = 0; idx < servers.num_servers; ++idx) {
        if ((connfd = client_connect(servers.servers[idx].port)) < 0) {
            servers.servers[idx].alive = false;
            continue;
        }
        if (send(connfd, healthcheck, strlen(healthcheck), 0) < 0) {
            servers.servers[idx].alive = false;
            close(connfd);
            continue;
        }
        ssize_t bytes;
        ssize_t t_bytes = 0;
        memset(&buff, '\0',BUFFER_SIZE);
        while (true) {
            bytes = recv(connfd, buff+t_bytes, BUFFER_SIZE - 1 , 0);
            if (bytes < 0) {
                servers.servers[idx].alive = false;
                break;
            }
            if (bytes == 0) break;
            t_bytes += bytes;
        }
        if (bytes < 0) continue;
        close(connfd);
        parse_healthcheck(idx, buff);
    }
}

/*
 * Initialize default values for the servers
 * argc : number of arguments
 * argv : array of arguments
 * server_port_idx : index of the first port number in argv
*/
int init_servers(int argc, char* argv[], int server_port_idx) {
    servers.num_servers = argc - server_port_idx;
    servers.servers = malloc(servers.num_servers * sizeof(ServerInfo));
    pthread_mutex_init(&servers.mut, NULL);
    pthread_cond_init(&servers.cond, NULL);
    pthread_mutex_init(&servers.health_mut, NULL);
    pthread_cond_init(&servers.health_cond, NULL);
    servers.requests = 0;
    for (int idx = server_port_idx; idx < argc; ++idx) {
        int sidx = idx - server_port_idx;
        servers.servers[sidx].port = atoi(argv[idx]);
        if(servers.servers[sidx].port == 0) return FAILURE;
        servers.servers[sidx].alive = false;
    }
    do_health_check();
    return SUCCESS;
}

/*
 * Parse the program arguments
 * argc : number of arguments
 * argv : array of arguments
*/
int parseArgs(int argc, char* argv[]) {
    int opt;

    // Get Flags
    while (true) {
        opt = getopt(argc, argv, "N:R:");
        if (opt == -1) break;              
        if (opt == 'N') servers.num_threads = atoi(optarg);
        else if (opt == 'R') servers.req_delay = atoi(optarg);
        else return FAILURE;
    }
    if (argc - optind < 2) return FAILURE;

    // Store port number and check if it is invalid
    servers.port = atoi(argv[optind++]);
    if (servers.port == 0) {
        return FAILURE;
    }
    if (init_servers(argc, argv, optind) == FAILURE) return FAILURE;
    return SUCCESS;
}

/*
 * select the best server according to the health status
*/
int select_server() {
    int port_idx = -1;
    int req = INT_MAX;
    for (int idx = 0; idx < servers.num_servers; ++idx) {
        if(!servers.servers[idx].alive) continue;
        if (servers.servers[idx].total_requests < req) {
            req = servers.servers[idx].total_requests;
            port_idx = idx;
        }
        else if (servers.servers[idx].total_requests == req) {
            if(servers.servers[idx].total_errors < servers.servers[port_idx].total_errors) 
                port_idx = idx;
        }
    }
    if (port_idx != -1) servers.servers[port_idx].total_requests += 1;
    return port_idx;
}


/*
 * Handle the request by connecting to the best server and forming 
 * a client server pair
*/
void* handle_request() {
    int acceptfd, connfd;
    while (1) {
        pthread_mutex_lock(&servers.mut);
        if((acceptfd = dequeue()) == INVALID) {
            pthread_cond_wait(&servers.cond, &servers.mut);
            acceptfd = dequeue();
        }
        pthread_mutex_unlock(&servers.mut);
        if(acceptfd != INVALID) {
            pthread_mutex_lock(&servers.health_mut);
            int port_idx = select_server();
            pthread_mutex_unlock(&servers.health_mut);
            if (port_idx == -1) {
                if (send(acceptfd, error, strlen(error), 0) < 0){
                    // warn("send_err\n");
                }
            }
            else {
                if ((connfd = client_connect(servers.servers[port_idx].port)) < 0) {
                    if (send(acceptfd, error, strlen(error), 0) < 0){
                        // warn("send_err\n");
                    }
                }
                else {
                    bridge_loop(acceptfd, connfd);
                    close(connfd);
                }
            }
            close(acceptfd);
        }
    } 
    return NULL;
}

/*
 * Handle health thread
*/
void* handle_health() {
    struct timespec timeout;
    while(1) {
        pthread_mutex_lock(&servers.health_mut);
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += WAIT_TIME;
        pthread_cond_timedwait(&servers.health_cond, &servers.health_mut, &timeout);
        do_health_check();
        pthread_mutex_unlock(&servers.health_mut);
    }
    return NULL;
}

/*
 * Main Thread
*/
int main(int argc,char **argv) {
    int listenfd, acceptfd;
    if (argc < 3) {
        return EXIT_FAILURE;
    }
    servers.num_threads = 4;
    servers.req_delay = 5;
    if (parseArgs(argc, argv) == FAILURE) {
        errno = EIO;
        // warn("Input\n");
        return EXIT_FAILURE;
    }

    if ((listenfd = server_listen(servers.port)) < 0) {
        // perror("failed listening");
        return EXIT_FAILURE;
    }
    
    pthread_t threads[servers.num_threads];
    for (int idx = 0; idx < servers.num_threads; ++idx) {
        pthread_create(&threads[idx], NULL, handle_request, NULL);
    }

    pthread_t health_thread;
    pthread_create(&health_thread, NULL, handle_health, NULL);

    while(true) {
        if ((acceptfd = accept(listenfd, NULL, NULL)) < 0)
            // perror("failed accepting");
        
        pthread_mutex_lock(&servers.mut);
        enqueue(acceptfd);
        pthread_cond_signal(&servers.cond);
        pthread_mutex_unlock(&servers.mut);

        servers.requests += 1;
        if ((servers.requests % servers.req_delay) == 0) {
            pthread_cond_signal(&servers.health_cond);
        }  

    }
}
