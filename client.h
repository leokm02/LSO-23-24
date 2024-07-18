#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
    int clientSocket;
    int clientId;
    int products;
} Client;

void simulateShopping(int customerId, int shoppingTime, int cashierIndex, int products);

#endif // CLIENT_H
