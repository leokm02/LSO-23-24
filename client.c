#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "constants.h"
#include "client.h"

char buffer[BUFFER_SIZE];

// Function to simulate shopping and communicate with the server
void simulateShopping(int customerId, int shoppingTime, int cashierIndex, int products) {


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

    int numbytes = recv(sock, &buffer, BUFFER_SIZE, 0);
    if (numbytes < 0) {
        perror("Communication error");
        exit(EXIT_FAILURE);
    }
    if(strstr(buffer, "Welcome") != NULL) {
            printf("Permission to enter received. Entering...\n");
        } else {
            printf("%d Waiting for permission.\n", customerId);
            memset(buffer, 0, BUFFER_SIZE);
            recv(sock, &buffer, BUFFER_SIZE, 0);
        }

    printf("Customer %d that took %d products is shopping for %d seconds.\n", customerId, products, shoppingTime);
    sleep(shoppingTime); // Simulate shopping time

    // Send number of products and customerId to the server
    send(sock, &products, sizeof(products), 0);
    send(sock, &customerId, sizeof(customerId), 0);

    char message[BUFFER_SIZE] = {0};

    recv(sock, &message, BUFFER_SIZE, 0);

    if(strstr(message, "Payment received") != NULL) {
        printf("Payment confirmed! %d Leaving...\n", customerId);
    } else if(strstr(message, "You can leave!") != NULL) {
        printf("Permission granted from supervisor! %d Leaving...\n", customerId);
    }



    // Close the socket
    close(sock);
}

int main(void) {
    srand(time(NULL)+getpid()); // Seed the random number generator
    int customerId = rand() % 1000; // Generate a random customer ID
    int shoppingTime = rand() % 5 + 1; // Generate a random shopping time
    int cashierIndex = rand() % ACTIVE_CASHIERS; // Choose a random cashier index (assuming 3 cashiers)
    int products = rand() % 10; // Generate a random number of products

    memset(buffer, 0, BUFFER_SIZE);

    simulateShopping(customerId, shoppingTime, cashierIndex, products);

    return 0;
}
