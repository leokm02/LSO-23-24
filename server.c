#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9090
#define MAX_CASHIERS 10
#define MAX_QUEUE_SIZE 100

// Structure to represent a Cashier
typedef struct {
    int id;
    int baseServiceTime;
    pthread_mutex_t queueMutex; // Mutex for controlling access to the cashier
} Cashier;

Cashier cashiers[MAX_CASHIERS]; // Array of cashiers
int cashierCount = 3; // Number of cashiers

// Function to handle each client
void* handleClient(void* arg) {
    int clientSocket = *((int*)arg); // Get client socket from argument
    free(arg);

    int cashierIndex, products, customerId;
    // Receive cashier index and number of products from client
    recv(clientSocket, &cashierIndex, sizeof(cashierIndex), 0);
    recv(clientSocket, &products, sizeof(products), 0);
    recv(clientSocket, &customerId, sizeof(customerId), 0);

    Cashier* cashier = &cashiers[cashierIndex]; // Get the corresponding cashier
    pthread_mutex_lock(&cashier->queueMutex); // Lock the cashier's queue

    // Calculate service time based on base service time and number of products
    int serviceTime = cashier->baseServiceTime + products;
    printf("Serving customer %d with %d products at cashier %d for %d seconds.\n",customerId, products, cashierIndex, serviceTime);
    sleep(serviceTime); // Simulate service time

    pthread_mutex_unlock(&cashier->queueMutex); // Unlock the cashier's queue
    printf("Done serving customer %d.\n", customerId);
    close(clientSocket); // Close the client socket
    return NULL;
}

int main() {
    srand(time(NULL)); // Seed the random number generator
    for (int i = 0; i < cashierCount; i++) {
        // Initialize each cashier with a random base service time and a mutex
        cashiers[i] = (Cashier){ .id = i, .baseServiceTime = rand() % 3 + 1 };
        pthread_mutex_init(&cashiers[i].queueMutex, NULL);
    }

    int serverSocket, newSocket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create a socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is running and waiting for connections on port %d...\n", PORT);

    // Main loop to accept and handle client connections
    while (1) {
        if ((newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the client
        pthread_t thread;
        int* pclient = malloc(sizeof(int));
        *pclient = newSocket;
        pthread_create(&thread, NULL, handleClient, pclient);
        pthread_detach(thread); // Detach the thread to allow it to clean up after itself
    }

    // Destroy the mutexes for each cashier
    for (int i = 0; i < cashierCount; i++) {
        pthread_mutex_destroy(&cashiers[i].queueMutex);
    }

    return 0;
}