#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include "queue.h"
#include "constants.h"
#include "cashier.h"

volatile sig_atomic_t keepRunning = 1;
pthread_mutex_t shop = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
Cashier cashiers[MAX_CASHIERS]; // Array of cashiers
Queue* waiting_to_enter;
Queue* clients;

//Signal handling
void signal_handler(int signal) {
    keepRunning = 0;
    printf("\nSignal %d received. Shutting down...\n", signal);
    // Destroy the mutexes for each cashier
    for (int i = 0; i < ACTIVE_CASHIERS; i++) {
        pthread_mutex_destroy(&cashiers[i].queueMutex);
        destroy_queue(cashiers[i].queue);
    }
    destroy_queue(clients);
    destroy_queue(waiting_to_enter);
    exit(EXIT_SUCCESS);
}

//Function to get cashier with shortest queue
int get_shortest_queue() {
    int cashier = 0;
    int queueSize = 0;
    int tmp = 0;
    for(int i=0; i < ACTIVE_CASHIERS; i++) {
        tmp = get_size(cashiers[i].queue);
        if(tmp==0) {
            return i;
        }
        if(tmp<queueSize) {
            queueSize = tmp;
            cashier = i;
        }
    }
    return cashier;
}

// Function to handle each client
void* handleClient(void* arg) {
    int clientSocket = *((int*)arg); // Get client socket from argument
    pthread_mutex_lock(&shop);
    if(get_size(clients) >= MAX_CLIENTS) {
        printf("%s%s%s Market is full, waiting for clients to leave...\n",YELLOW_COLOR, "[WARN]", RESET_COLOR);
        // Clients should wait to enter if capacity is reached
        enqueue(waiting_to_enter, arg);
        do {
            pthread_cond_wait(&cond, &shop);
            if(get_size(waiting_to_enter)+get_size(clients) < MAX_CLIENTS) {
                break;
            }
        } while(get_size(clients) >= MAX_CLIENTS-CLIENTS_BATCH_SIZE);
        dequeue(waiting_to_enter);
        printf("%s%s%s Now %d can enter\n",GREEN_COLOR, "[INFO]", RESET_COLOR, clientSocket);
    }
    send(clientSocket, &WELCOME_MESSAGE, BUFFER_SIZE, 0);
    enqueue(clients, arg);
    pthread_mutex_unlock(&shop);

    char message[100] = "Payment received, Bye bye!";

    int cashierIndex, products, customerId;
    // Receive number of products and customerID from client
    recv(clientSocket, &products, sizeof(products), 0);
    recv(clientSocket, &customerId, sizeof(customerId), 0);

    cashierIndex = get_shortest_queue(); // Get cashier with shortest queue

    Cashier* cashier = &cashiers[cashierIndex]; // Get the corresponding cashier
    pthread_mutex_lock(&cashier->queueMutex); // Lock the cashier's queue

    // Calculate service time based on base service time and number of products
    int serviceTime = cashier->baseServiceTime + products;
    printf("%s%s%s Serving customer %d with %d products at cashier %d for %d seconds.\n", GREEN_COLOR, "[INFO]", RESET_COLOR, customerId, products, cashierIndex, serviceTime);
    enqueue(cashier->queue, arg); // Enqueue client
    sleep(serviceTime); // Simulate service time
    dequeue(cashier->queue); //Dequeue client
    pthread_mutex_unlock(&cashier->queueMutex); // Unlock the cashier's queue
    printf("%s%s%s Done serving customer %d.\n",GREEN_COLOR, "[INFO]", RESET_COLOR, customerId);
    send(clientSocket, &message, sizeof(message), 0);
    close(clientSocket); // Close the client socket
    dequeue(clients);
    pthread_cond_broadcast(&cond); // Notify all waiting threads to wake up
    printf("%s%s%s There are %d customers shopping\n",GREEN_COLOR, "[INFO]", RESET_COLOR, get_size(clients));
    if(get_size(waiting_to_enter) > 0) {
        printf("%s%s%s There are %d customers waiting to enter\n",GREEN_COLOR, "[INFO]", RESET_COLOR, get_size(waiting_to_enter));
    }
    free(arg);
    return NULL;
}

int main() {
    system("clear");
    printf("%s", LOGO);
    clients = create_queue();
    waiting_to_enter = create_queue();
    signal(SIGINT, signal_handler);
    srand(time(NULL)+getpid()); // Seed the random number generator    
    for (int i = 0; i < ACTIVE_CASHIERS; i++) {
        // Initialize each cashier with a random base service time and a mutex
        cashiers[i] = (Cashier){ .id = i, .baseServiceTime = rand() % 3 + 1 };
        pthread_mutex_init(&cashiers[i].queueMutex, NULL);
        cashiers[i].queue = create_queue();
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
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Set server socket to non-blocking mode
    // int flags = fcntl(serverSocket, F_GETFL, 0);
    // fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);

    printf("%s%s%s Server is running and waiting for connections on port %d...\n", GREEN_COLOR, "[INFO]", RESET_COLOR, PORT);

    // Main loop to accept and handle client connections
    while (keepRunning) {
        newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (newSocket < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // No pending connections; sleep for a short time and try again
                usleep(100000); // Sleep for 100ms
                if(keepRunning) {
                    continue;
                } else {
                    break;
                }
            } else {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
        }
        
        
        printf("%s%s%s New client connected: %d\n",GREEN_COLOR, "[INFO]", RESET_COLOR, newSocket);
        printf("%s%s%s Clients in the market %d, Clients waiting to enter %d\n",GREEN_COLOR, "[INFO]", RESET_COLOR, get_size(clients), get_size(waiting_to_enter));
        // Create a new thread to handle the client
        pthread_t thread;
        int* pclient = malloc(sizeof(int));
        *pclient = newSocket;
        pthread_create(&thread, NULL, handleClient, pclient);
        pthread_detach(thread); // Detach the thread to allow it to clean up after itself
    }

    return 0;
}