#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 9090

// Function to simulate shopping and communicate with the server
void simulateShopping(int customerId, int shoppingTime, int cashierIndex, int products) {
    printf("Customer %d that took %d products is shopping for %d seconds.\n", customerId, products, shoppingTime);
    sleep(shoppingTime); // Simulate shopping time

    int sock = 0;
    struct sockaddr_in serv_addr;
    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    // Set up the address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return;
    }

    // Send cashier index, number of products and customerId to the server
    send(sock, &cashierIndex, sizeof(cashierIndex), 0);
    send(sock, &products, sizeof(products), 0);
    send(sock, &customerId, sizeof(customerId), 0);

    // Close the socket
    close(sock);
}

int main() {
    srand(time(NULL)); // Seed the random number generator
    int customerId = rand() % 1000; // Generate a random customer ID
    int shoppingTime = rand() % 5 + 1; // Generate a random shopping time
    int cashierIndex = rand() % 3; // Choose a random cashier index (assuming 3 cashiers)
    int products = rand() % 10; // Generate a random number of products

    simulateShopping(customerId, shoppingTime, cashierIndex, products);

    return 0;
}