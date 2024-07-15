#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

// Function to create a queue
Queue* create_queue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (!queue) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = NULL;
    queue->size = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}

// Function to enqueue data
void enqueue(Queue* queue, void* data) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        perror("Failed to allocate node");
        exit(EXIT_FAILURE);
    }
    new_node->data = data;
    new_node->next = NULL;

    pthread_mutex_lock(&queue->lock);

    if (queue->rear == NULL) {
        queue->front = queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }

    queue->size++;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}

// Function to dequeue data
void* dequeue(Queue* queue) {
    pthread_mutex_lock(&queue->lock);

    while (queue->front == NULL) {
        pthread_cond_wait(&queue->cond, &queue->lock);
    }

    Node* temp = queue->front;
    void* data = temp->data;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    queue->size--;

    free(temp);
    pthread_mutex_unlock(&queue->lock);
    return data;
}

// Function to check if the queue is empty
int is_empty(Queue* queue) {
    pthread_mutex_lock(&queue->lock);
    int empty = (queue->front == NULL);
    pthread_mutex_unlock(&queue->lock);
    return empty;
}

// Function to get the size of the queue
int get_size(Queue* queue) {
    pthread_mutex_lock(&queue->lock);
    int size = queue->size;
    pthread_mutex_unlock(&queue->lock);
    return size;
}

// Function to destroy the queue
void destroy_queue(Queue* queue) {
    if(queue == NULL) { return; }
    pthread_mutex_lock(&queue->lock);

    Node* current = queue->front;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }

    pthread_mutex_unlock(&queue->lock);
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->cond);
    free(queue);
}