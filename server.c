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
#include "client.h"

volatile sig_atomic_t keepRunning = 1;
pthread_mutex_t shop = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
Cashier cashiers[MAX_CASHIERS]; // Array of cashiers
char timeLog[BUFFER_SIZE];
Queue* waiting_to_enter;
Queue* clients;
Queue* waiting_to_exit;


#if MAX_CASHIERS <= 0 || ACTIVE_CASHIERS <= 0 || MAX_CLIENTS <= 0 || CLIENTS_BATCH_SIZE <= 0
#error "Error, one or more value provided are 0 or negative"
#endif

#if MAX_CASHIERS < ACTIVE_CASHIERS
#error "Error, MAX_CASHIERS should be greater or equal than ACTIVE_CASHIERS"
#endif

#if MAX_CLIENTS <= CLIENTS_BATCH_SIZE
#error "Error, MAX_CLIENTS should be greater than CLIENTS_BATCH_SIZE
#endif

void writeCurrentTimeToBuffer(char *timeBuffer) {
    time_t rawtime;
    struct tm *timeinfo;
    
    // Get the current time
    time(&rawtime);
    
    // Convert the current time to local time representation
    timeinfo = localtime(&rawtime);
    
    // Format the time and write it to the provided buffer
    strftime(timeBuffer, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", timeinfo);
}

//Signal handling
void signal_handler(int signal) {
    keepRunning = 0;
    printf("\nSignal %d received. Shutting down...\n", signal);
    // Destroy the mutexes for each cashier
    for (int i = 0; i < ACTIVE_CASHIERS; i++) {
        // pthread_mutex_destroy(&cashiers[i].queueMutex);
        destroy_queue(cashiers[i].queue);
    }
    destroy_queue(clients);
    destroy_queue(waiting_to_enter);
    exit(EXIT_SUCCESS);
}

//Function to get cashier with shortest queue
int get_shortest_queue(void) {
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
        printf("%s%s%s Clients in the market %d, Clients waiting to enter %d\n",GREEN_COLOR, "[INFO]", RESET_COLOR, get_size(clients), get_size(waiting_to_enter));
        do {
            pthread_cond_wait(&cond, &shop); // wait until another client leaves
        } while(get_size(waiting_to_enter)+get_size(clients) > MAX_CLIENTS); 
        dequeue(waiting_to_enter);
        printf("%s%s%s Now %d can enter\n",GREEN_COLOR, "[INFO]", RESET_COLOR, clientSocket);
    }
    send(clientSocket, &WELCOME_MESSAGE, BUFFER_SIZE, 0);
    enqueue(clients, arg);
    pthread_mutex_unlock(&shop);

    int cashierIndex, products, customerId;
    // Receive number of products and customerID from client
    recv(clientSocket, &products, sizeof(products), 0);
    recv(clientSocket, &customerId, sizeof(customerId), 0);

    Client* client = malloc(sizeof(Client));
    client->clientSocket = clientSocket;
    client->products = products;
    if(products > 0) { //If client bought something enqueue it to a cashier
        cashierIndex = get_shortest_queue(); // Get cashier with shortest queue
        Cashier* cashier = &cashiers[cashierIndex]; // Get the corresponding cashier
        enqueue(cashier->queue, client); // Enqueue client to the cashier
    } else {
        enqueue(waiting_to_exit, client); // Enqueue client to the supervisor queue
    }
    free(arg);
    return NULL;
}

void* cashier_logic(void* arg) {
    const char message[100] = "Payment received, You can leave!";
    char timeBuffer[BUFFER_SIZE] = {0};
    Cashier* cashier = (Cashier*) arg;
    printf("%s%s%s Created thread for cashier %d\n", GREEN_COLOR, "[INFO]", RESET_COLOR, cashier->id);
    // int client = *((int*)arg);
    while(1) {
        if(!is_empty(cashier->queue)) {
            writeCurrentTimeToBuffer(timeBuffer);
            Client* client = ((Client*)dequeue(cashier->queue));
            int serviceTime = cashier->baseServiceTime + client->products;
            printf("%s%s %s %s%d%s%s Serving customer %d with %d products, it will take %d seconds.\n", GREEN_COLOR, timeBuffer, "[INFO]", "[CASH-", cashier->id+1, "]", RESET_COLOR, client->clientSocket, client->products, serviceTime);
            sleep(serviceTime); // Simulate service time
            writeCurrentTimeToBuffer(timeBuffer);
            printf("%s%s %s %s%d%s%s Done serving customer %d.\n",GREEN_COLOR, timeBuffer, "[INFO]", "[CASH-", cashier->id+1, "]", RESET_COLOR, client->clientSocket);
            send(client->clientSocket, &message, sizeof(message), 0);
            close(client->clientSocket); // Close the client socket
            dequeue(clients); //Client exits the market
            pthread_cond_broadcast(&cond); // Notify all waiting clients threads to wake up
        }
        usleep(500000);
        memset(timeBuffer, 0, BUFFER_SIZE);
    }
    return NULL;
}

void* supervisor_logic(void* arg) {
    const char message[BUFFER_SIZE] = "Permission granted, You can leave!";
    char timeBuffer[BUFFER_SIZE] = {0};
    printf("%s%s%s Created thread for supervisor\n", GREEN_COLOR, "[INFO]", RESET_COLOR);
    while(1) {
        if(!is_empty(waiting_to_exit)) {
            Client* client = ((Client*)dequeue(waiting_to_exit));
            int randomTime = rand() % 6 + 1;
            writeCurrentTimeToBuffer(timeBuffer);
            printf("%s%s %s %s%s Checking if customer %d stole something...\n", GREEN_COLOR, timeBuffer, "[INFO]", "[SUPERV]", RESET_COLOR, client->clientSocket);
            sleep(randomTime);
            writeCurrentTimeToBuffer(timeBuffer);
            printf("%s%s %s %s%s All good %d can go\n", GREEN_COLOR, timeBuffer, "[INFO]", "[SUPERV]", RESET_COLOR, client->clientSocket);
            send(client->clientSocket, &message, sizeof(message), 0);
            close(client->clientSocket); // Close the client socket
            dequeue(clients); //Client exits the market
            pthread_cond_broadcast(&cond); // Notify all waiting clients threads to wake up
        }
        usleep(500000);
        memset(timeBuffer, 0, BUFFER_SIZE);
    }
    return NULL;
}

int main(void) {
    system("clear");
    printf("%s", LOGO);
    clients = create_queue();
    waiting_to_enter = create_queue();
    waiting_to_exit = create_queue();
    signal(SIGINT, signal_handler);
    srand(time(NULL)+getpid()); // Seed the random number generator    
    for (int i = 0; i < ACTIVE_CASHIERS; i++) {
        // Initialize each cashier with a random base service time and a mutex
        cashiers[i] = (Cashier){ .id = i, .baseServiceTime = rand() % 3 + 1 };
        // pthread_mutex_init(&cashiers[i].queueMutex, NULL);
        cashiers[i].queue = create_queue();
        pthread_t cashier_thread;
        pthread_create(&cashier_thread, NULL, cashier_logic, &cashiers[i]);
        pthread_detach(cashier_thread);
   }

   pthread_t supervisor_thread;
   pthread_create(&supervisor_thread, NULL, supervisor_logic, NULL);

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

    printf("%s%s%s Server is running and waiting for connections on port %d...\n", GREEN_COLOR, "[INFO]", RESET_COLOR, PORT);

    // Main loop to accept and handle client connections
    while (keepRunning) {
        newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen);       
        printf("%s%s%s New client connected: %d\n",GREEN_COLOR, "[INFO]", RESET_COLOR, newSocket);
        // Create a new thread to handle the client
        pthread_t thread;
        int* pclient = malloc(sizeof(int));
        *pclient = newSocket;
        pthread_create(&thread, NULL, handleClient, pclient);
        pthread_detach(thread);
    }
    return 0;
}
