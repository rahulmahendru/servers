#include "server.h"

// Prompt definitions
static const char* prompt_wait = "[+] server is waiting...\n";
static const char* prompt_sent = "Response Sent\n";

// Main Function
int main(int argc, char** argv) {

    // Check for invalid call to program
    if (argc < 2) {
        perror("Input");
        return EXIT_FAILURE;
    }

    // Create sockaddr_in with server information
    char* port = argv[1];
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
    ret = listen(server_sockd, 5); // 5 should be enough, if not use SOMAXCONN

    // Check for listen error
    if (ret < 0) {
        perror("Listen");
        return EXIT_FAILURE;
    }

    // Connecting with a client
    struct sockaddr client_addr;
    socklen_t client_addrlen;

    // httpObject to store resonse details
    struct httpObject message;

    // Loop to accept client request
    while (true) {
        memset(&message, 0, sizeof(message));
	    memset(&client_addrlen, 0, sizeof(client_addrlen));

        // Prompt waiting for client
        ssize_t bytes_r = write(1, prompt_wait, strlen(prompt_wait));
        if (bytes_r < 0){
            perror("write");
            return EXIT_FAILURE;
        }

        // Accept Connection
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);

        // Check for accept error
        if (client_sockd < 0) {
            perror("Accept");
            return EXIT_FAILURE;
        }

        // Read HTTP Message
        read_http_response(client_sockd, &message);

        // Check for errors from reading response
        if (message.status_code != OK) {
            send_error_response(client_sockd, &message);
            ssize_t bytes_p = write(1, prompt_sent, strlen(prompt_sent));     // Prompt Sent Response
            if (bytes_p < 0){
                perror("write");
                return EXIT_FAILURE;
            }
            close(client_sockd);
            continue;
        }

        // Process Request
        process_request(client_sockd, &message);

        // Check for errors from processing request
        if (message.status_code != OK && message.status_code != CREATED) {
            send_error_response(client_sockd, &message);
            ssize_t bytes_p = write(1, prompt_sent, strlen(prompt_sent));       // Prompt sent response
            if (bytes_p < 0) {
                perror("write");
                return EXIT_FAILURE;
            }
            close(client_sockd);
            continue;
        }
        
        // Prompt sent response
        ssize_t bytes_p = write(1, prompt_sent, strlen(prompt_sent));
        if (bytes_p < 0) {
            perror("write");
            return EXIT_FAILURE;
        }

        // Close client
        close(client_sockd);
    }
    // Close server
    close (server_sockd);

    return EXIT_SUCCESS;
}
