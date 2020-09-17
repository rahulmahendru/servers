# Servers

This repository is a collection of basic level HTTP server modules which include a single threaded server, a multi-threaded server and a load balancer. All of these were implemented as part of class assignments in a Introduction to Operating Systems class.

### Single Threaded Server
This version of the server is created to handle a single thread at a time. The module responds to the PUT, GET and HEAD commands to write, read and provide information about the files respectively.

### Multi Threaded Server
This version of the server is created to handle a multiple threads at a time. Apart from the operations od a single threaded server, this version of the server performs logging to record each request and data dump as hex. The server also supports a health check feature to request the working performance of the server.

