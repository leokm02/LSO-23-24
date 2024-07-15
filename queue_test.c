#include <stdio.h>
#include <stdlib.h>
#include "queue.c"

// Sample data structure
typedef struct {
    int id;
    char name[20];
} Data;

void* producer(void* arg) {
    Queue* queue = (Queue*)arg;
    for (int i = 0; i < 5; ++i) {
        Data* data = (Data*)malloc(sizeof(Data));
        data->id = i;
        snprintf(data->name, 20, "Name%d", i);
        enqueue(queue, data);
        printf("Produced: %d, %s (Queue size: %d)\n", data->id, data->name, get_size(queue));
    }
    return NULL;
}

void* consumer(void* arg) {
    Queue* queue = (Queue*)arg;
    for (int i = 0; i < 5; ++i) {
        Data* data = (Data*)dequeue(queue);
        printf("Consumed: %d, %s (Queue size: %d)\n", data->id, data->name, get_size(queue));
        free(data);
    }
    return NULL;
}

int main() {
    Queue* queue = create_queue();

    pthread_t prod_thread, cons_thread;

    pthread_create(&prod_thread, NULL, producer, (void*)queue);
    pthread_create(&cons_thread, NULL, consumer, (void*)queue);


    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    destroy_queue(queue);

    return 0;
}