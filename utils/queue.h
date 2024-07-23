#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

// Node structure to store data
typedef struct Node {
    void* data;
    struct Node* next;
} Node;

// Queue structure
typedef struct Queue {
    Node* front;
    Node* rear;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} Queue;

// Function to create a queue
Queue* create_queue(void);

// Function to enqueue data
void enqueue(Queue* queue, void* data);

// Function to dequeue data
void* dequeue(Queue* queue);

// Function to check if the queue is empty
int is_empty(Queue* queue);

// Function to get the size of the queue
int get_size(Queue* queue);

// Function to destroy the queue
void destroy_queue(Queue* queue);

#endif // QUEUE_H
