# Single Threaded Server

The single threaded server is created to handle a single thread at a time. The module responds to the PUT, GET and HEAD commands to write, read and provide information about the files respectively.

The **'httpserver'** program can be built by running the `make` command on the command terminal.
In order to run the program, call the executable file **'httpserver'** followed by the port number as the program argument. For eg.
> httpserver 8080
where 8080 is the port number. Multiple program arguments are not allowed. The server is then ready to respond to client requests.
The server is single-threaded. What this means is that it can only work on one request at a time. After working on one request, it waits to recieve the next request from the client.

### Bugs
The program does not encounter any bugs at the moment.
