#include "queue.h"

node_t *head = NULL;
node_t *tail = NULL;

node_t* newNode(int client_socket) { 
    node_t* temp = malloc(sizeof(node_t)); 
    temp->client_socket = client_socket; 
    temp->next = NULL; 
    return temp; 
} 

void enqueue(int client_socket) {
    node_t* new_node = newNode(client_socket);
    if (tail == NULL) head = new_node;
    else tail->next = new_node;
    tail = new_node;
}

int dequeue() {
    if (head != NULL) {
        int client_socket = head->client_socket;
        node_t *temp = head;
        head = head->next;
        free(temp);
        if (head == NULL) tail = NULL;
        return client_socket;
    }
    return -1;
}
