# Load Balancer

The load balancer distributes connections over a set of servers after accepting connections from the clients. The selection of the servers is based on the perofrmance of the servers using the `healthcheck` functionality of the multi-threaded server. 
The 'loadbalancer' program can be built by running the make command on the command terminal. In order to run the program, call the executable file 'loadbalancer' followed by the port number as the program argument. In addition to the port number, the number of parallel connections may be specified by using the format `-N num_conn` where num_conn is the number of threads. If not specified a default of 4 parallel connections will be used. Another argument may be added to specify the number of requests after which a healthcheck is requested for the servers. In order to do use use `-R requests` where requests is the number of requests. By default, a healthcheck will be sent after 5 requests or 3 seconds which ever comes first. 

Bugs:
There may be probable heisenbugs in the execution, which are evident in very few executions of the program. 