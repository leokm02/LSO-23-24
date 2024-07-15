#ifndef CASHIER_H
#define CASHIER_H

// Structure to represent a Cashier
typedef struct {
    int id;
    int baseServiceTime;
    Queue* queue;
    pthread_mutex_t queueMutex; // Mutex for controlling access to the cashier
} Cashier;

#endif // CASHIER_H