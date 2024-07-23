#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
    int clientSocket;
    int clientId;
    int products;
} Client;

int setupConnection(void);
void goShopping(int sock, int customerId, int cashierIndex);
void simulateShopping(int sock, int customerId, int shoppingTime, int cashierIndex, int products);

#endif // CLIENT_H
