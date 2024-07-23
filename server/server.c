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
#include "../utils/queue.h"
#include "../parameters/parameters.h"
#include "../utils/constants.h"
#include "../utils/cashier.h"
#include "../client/client.h"
#include "../utils/logger.h"


volatile sig_atomic_t keepRunning = 1;
pthread_mutex_t shop = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
Cashier cashiers[MAX_CASHIERS]; 
char time_log[BUFFER_SIZE];
char log_message_buf[BUFFER_SIZE];
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
    writeCurrentTimeToBuffer(time_log);
    printf("%s %s%s%s Signal %d received. Shutting down...\n", time_log, GREEN_COLOR, "[INFO]", RESET_COLOR, signal);
    snprintf(log_message_buf, BUFFER_SIZE, "%s %s Signal %d received. Shutting down...\n", time_log, "[INFO]", signal);
    log_message(log_message_buf);
    for (int i = 0; i < ACTIVE_CASHIERS; i++) {
        destroy_queue(cashiers[i].queue);
    }
    destroy_queue(clients);
    destroy_queue(waiting_to_enter);
    destroy_logger();
    pthread_mutex_destroy(&shop);
    pthread_cond_destroy(&cond);
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

char* readProducts(void) {
    int fd = open("./utils/products.txt", O_RDONLY); // Open the file in read-only mode
    if (fd == -1) {
        perror("Unable to open file!"); 
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    int bytesRead;
    size_t totalSize = 0;
    size_t bufferSize = BUFFER_SIZE;

    // Allocate initial memory for the content
    char *content = (char *)malloc(bufferSize);
    if (content == NULL) {
        perror("Unable to allocate memory!");
        close(fd);
        exit(1);
    }

    // Initialize the content with an empty string
    content[0] = '\0';

    // Read the file content in chunks
    while ((bytesRead = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the buffer to make it a valid C string

        // Calculate new total size
        totalSize += bytesRead;

        // Reallocate memory if necessary
        if (totalSize >= bufferSize) {
            bufferSize *= 2;
            content = (char *)realloc(content, bufferSize);
            if (content == NULL) {
                perror("Unable to reallocate memory!");
                close(fd);
                exit(1);
            }
        }

        // Concatenate the new chunk to the content
        strcat(content, buffer);
    }

    if (bytesRead == -1) {
        perror("Error reading file!"); // Handle error if read fails
        free(content);
        close(fd); // Close the file descriptor
        exit(1);
    }

    close(fd); // Close the file descriptor
    return content;
}

// Function to handle each client
void* handleClient(void* arg) {
    char timeBuffer[BUFFER_SIZE] = {0};
    int clientSocket = *((int*)arg); // Get client socket 
    pthread_mutex_lock(&shop);
    if(get_size(clients) >= MAX_CLIENTS || get_size(waiting_to_enter) > 0) {
        writeCurrentTimeToBuffer(timeBuffer);
        printf("%s %s%s%s Market is full, waiting for clients to leave...\n", timeBuffer, YELLOW_COLOR, "[WARN]", RESET_COLOR);
        snprintf(log_message_buf, BUFFER_SIZE, "%s %s Market is full, waiting for clients to leave...", timeBuffer, "[WARN]");
        log_message(log_message_buf);
        // Clients should wait to enter if capacity is reached
        enqueue(waiting_to_enter, arg);
        send(clientSocket, "Wait", sizeof("Wait"), 0);
        writeCurrentTimeToBuffer(timeBuffer);
        printf("%s %s%s%s Clients in the market %d, Clients waiting to enter %d\n", timeBuffer, GREEN_COLOR, "[INFO]", RESET_COLOR, get_size(clients), get_size(waiting_to_enter));
        snprintf(log_message_buf, BUFFER_SIZE, "%s %s Clients in the market %d, Clients waiting to enter %d", timeBuffer, "[INFO]", get_size(clients), get_size(waiting_to_enter));
        log_message(log_message_buf);
        do {
            pthread_cond_wait(&cond, &shop); // wait until another client leaves
        } while(get_size(clients)+1 > MAX_CLIENTS && get_size(clients) > MAX_CLIENTS-CLIENTS_BATCH_SIZE); 
        dequeue(waiting_to_enter);
        writeCurrentTimeToBuffer(timeBuffer);
        printf("%s %s%s%s Now %d can enter\n", timeBuffer, GREEN_COLOR, "[INFO]", RESET_COLOR, clientSocket);
        snprintf(log_message_buf, BUFFER_SIZE, "%s %s Now %d can enter", timeBuffer, "[INFO]", clientSocket);
        log_message(log_message_buf);
    }
    send(clientSocket, &WELCOME_MESSAGE, BUFFER_SIZE, 0);
    enqueue(clients, arg);
    pthread_mutex_unlock(&shop);
    // Send list of products to client
    send(clientSocket, readProducts(), BUFFER_SIZE, 0);
    int cashierIndex, products, customerId;
    // Receive number of products and customerID from client
    recv(clientSocket, &products, sizeof(products), 0);
    recv(clientSocket, &customerId, sizeof(customerId), 0);

    Client* client = malloc(sizeof(Client));
    client->clientSocket = clientSocket;
    client->products = products;

    if(products > 0) { //If client bought something enqueue it to a cashier
        cashierIndex = get_shortest_queue(); 
        Cashier* cashier = &cashiers[cashierIndex]; 
        enqueue(cashier->queue, client); // Enqueue client to the cashier
    } else {
        enqueue(waiting_to_exit, client); // Enqueue client to the supervisor queue
    }
    free(arg);
    return NULL;
}

void* cashier_logic(void* arg) {
    const char message[] = "Payment received, You can leave!";
    char timeBuffer[BUFFER_SIZE] = {0};
    Cashier* cashier = (Cashier*) arg;

    while(1) {
        if(!is_empty(cashier->queue)) {
            writeCurrentTimeToBuffer(timeBuffer);
            Client* client = ((Client*)dequeue(cashier->queue));
            int serviceTime = cashier->baseServiceTime + client->products;
            printf("%s%s %s%s %s%d%s%s Serving customer %d with %d products, it will take %d seconds.\n", timeBuffer, GREEN_COLOR, "[INFO]", MAGENTA_COLOR, "[CASH-", cashier->id+1, "]", RESET_COLOR, client->clientSocket, client->products, serviceTime);
            snprintf(log_message_buf, BUFFER_SIZE, "%s %s %s%d%s Serving customer %d with %d products, it will take %d seconds.", timeBuffer, "[INFO]", "[CASH-", cashier->id+1, "]", client->clientSocket, client->products, serviceTime);
            log_message(log_message_buf);
            sleep(serviceTime); // Simulate service time
            writeCurrentTimeToBuffer(timeBuffer);
            printf("%s%s %s%s %s%d%s%s Done serving customer %d.\n", timeBuffer, GREEN_COLOR, "[INFO]", MAGENTA_COLOR, "[CASH-", cashier->id+1, "]", RESET_COLOR, client->clientSocket);
            snprintf(log_message_buf, BUFFER_SIZE, "%s %s %s%d%s Done serving customer %d.", timeBuffer, "[INFO]", "[CASH-", cashier->id+1, "]", client->clientSocket);
            log_message(log_message_buf);
            send(client->clientSocket, &message, sizeof(message), 0);
            close(client->clientSocket); // Close the client socket
            dequeue(clients); // Client exits the market
            if (get_size(clients) <= MAX_CLIENTS-CLIENTS_BATCH_SIZE && get_size(waiting_to_enter) > 0) { // C-E and someone is waiting to enter
                pthread_cond_broadcast(&cond); // Notify all waiting clients threads to wake up
            }
        }
        usleep(500000);
        memset(timeBuffer, 0, BUFFER_SIZE);
    }
    return NULL;
}

void* supervisor_logic(void* arg) {
    const char message[] = "Permission granted, You can leave!";
    char timeBuffer[BUFFER_SIZE] = {0};

    while(1) {
        if(!is_empty(waiting_to_exit)) {
            Client* client = ((Client*)dequeue(waiting_to_exit));
            int randomTime = rand() % 6 + 1;
            writeCurrentTimeToBuffer(timeBuffer);
            printf("%s%s %s%s %s%s Checking if customer %d stole something...\n", timeBuffer, GREEN_COLOR,  "[INFO]", MAGENTA_COLOR, "[SUPERV]", RESET_COLOR, client->clientSocket);
            snprintf(log_message_buf, BUFFER_SIZE, "%s %s %s Checking if customer %d stole something...", timeBuffer, "[INFO]", "[SUPERV]", client->clientSocket);
            log_message(log_message_buf);
            sleep(randomTime);
            writeCurrentTimeToBuffer(timeBuffer);
            printf("%s%s %s%s %s%s All good, %d can go\n", timeBuffer, GREEN_COLOR, "[INFO]", MAGENTA_COLOR, "[SUPERV]", RESET_COLOR, client->clientSocket);
            snprintf(log_message_buf, BUFFER_SIZE, "%s %s %s All good, %d can go", timeBuffer, "[INFO]", "[SUPERV]", client->clientSocket);
            log_message(log_message_buf);
            send(client->clientSocket, &message, sizeof(message), 0);
            close(client->clientSocket); // Close the client socket
            dequeue(clients); //Client exits the market
            if (get_size(clients) <= MAX_CLIENTS-CLIENTS_BATCH_SIZE && get_size(waiting_to_enter) > 0) { // C-E and someone is waiting to enter
                pthread_cond_broadcast(&cond); // Notify all waiting clients threads to wake up
            }
        }
        usleep(500000);
        memset(timeBuffer, 0, BUFFER_SIZE);
    }
    return NULL;
}

int main(void) {
    char timeBuffer[BUFFER_SIZE] = {0};
    system("clear");
    printf("%s", LOGO);
    clients = create_queue();
    waiting_to_enter = create_queue();
    waiting_to_exit = create_queue();
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    srand(time(NULL)+getpid()); // Seed the random number generator    
    for (int i = 0; i < ACTIVE_CASHIERS; i++) {
        // Initialize each cashier with a random base service time
        cashiers[i] = (Cashier){ .id = i, .baseServiceTime = rand() % 3 + 1 };
        cashiers[i].queue = create_queue();
        pthread_t cashier_thread;
        pthread_create(&cashier_thread, NULL, cashier_logic, &cashiers[i]);
        pthread_detach(cashier_thread);
        writeCurrentTimeToBuffer(timeBuffer);
        printf("%s%s %s%s Created thread for cashier %d\n", timeBuffer, GREEN_COLOR, "[INFO]", RESET_COLOR, i);
        snprintf(log_message_buf, BUFFER_SIZE, "%s %s Created thread for cashier %d", timeBuffer, "[INFO]", i);
        log_message(log_message_buf);
   }
    usleep(100);
    pthread_t supervisor_thread;
    pthread_create(&supervisor_thread, NULL, supervisor_logic, NULL);
    pthread_detach(supervisor_thread);
    writeCurrentTimeToBuffer(timeBuffer);
    printf("%s%s %s%s Created thread for supervisor\n",timeBuffer, GREEN_COLOR, "[INFO]", RESET_COLOR);
    snprintf(log_message_buf, BUFFER_SIZE, "%s %s Created thread for supervisor", timeBuffer, "[INFO]");
    log_message(log_message_buf);

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
    writeCurrentTimeToBuffer(timeBuffer);
    printf("%s%s %s%s Server is running and waiting for connections on port %d...\n", timeBuffer, GREEN_COLOR, "[INFO]", RESET_COLOR, PORT);
    snprintf(log_message_buf, BUFFER_SIZE, "%s %s Server is running and waiting for connections on port %d...", timeBuffer, "[INFO]", PORT);
    log_message(log_message_buf);

    // Main loop to accept and handle client connections
    while (keepRunning) {
        memset(timeBuffer, 0, BUFFER_SIZE);
        newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        writeCurrentTimeToBuffer(timeBuffer);       
        printf("%s %s%s%s New client connected: %d\n", timeBuffer, GREEN_COLOR, "[INFO]", RESET_COLOR, newSocket);
        snprintf(log_message_buf, BUFFER_SIZE, "%s %s New client connected: %d", timeBuffer, "[INFO]", newSocket);
        log_message(log_message_buf);
        // Create a new thread to handle the client
        pthread_t thread;
        int* pclient = malloc(sizeof(int));
        *pclient = newSocket;
        pthread_create(&thread, NULL, handleClient, pclient);
        pthread_detach(thread);
    }
    return 0;
}
