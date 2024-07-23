#ifndef CASHIER_H
#define CASHIER_H

// Structure to represent a Cashier
typedef struct {
    int id;
    int baseServiceTime;
    Queue* queue;
} Cashier;

#endif // CASHIER_H
