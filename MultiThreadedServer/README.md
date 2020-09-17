# Multi Threaded Server

The multi-threaded server is created to handle a multiple threads at a time. Apart from the operations od a single threaded server, this version of the server performs logging to record each request and data dump as hex. The server also supports a health check feature to request the working performance of the server.

The **'server'** program can be built by running the `make` command on the command terminal.
In order to run the program, call the executable file **'server'** followed by the port number as the program argument. For eg.
``httpserver 8080``
where 8080 is the port number. You may also provide an additional argument to execute the logging feature, using the `-l` flag. This may be done as follows: `httpserver 8080 -l log_file`, where log_file is the name of the file where you want to store the log. The program may also accept a `-N` flag to specify the number of threads that may work simultaneously in the server. This may be done as follows: `httpserver 8080 -N num_threads`, where num_thread is the number of threads specified. By default, num_threads is 4. 

### Bugs
No Bugs have been observed. 
