#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

struct node {
    int client_socket;
    struct node* next;
};

typedef struct node node_t;

node_t* newNode(int client_socket);
void enqueue(int client_socket);
int dequeue();

#endif