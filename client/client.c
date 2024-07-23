#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <termios.h>
#include "../utils/constants.h"
#include "client.h"
#include "../utils/queue.h"


char buffer[BUFFER_SIZE];
Queue* basket;


void enable_raw_mode(void) {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);
}

void disable_raw_mode(void) {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
}

char getch(void) {
    char buf = 0;
    if (read(0, &buf, 1) < 0)
        return -1;
    return buf;
}

void printBasket(Queue* basket) {

}

int choose_product(char productsList[], int page_size) {
    // Count the number of products
    int num_products = 0;
    for (char *ptr = productsList; *ptr != '\0'; ptr++) {
        if (*ptr == '\n') num_products++;
    }

    // Allocate memory for product pointers
    char **products = malloc(num_products * sizeof(char *));
    if (!products) {
        perror("Failed to allocate memory");
        exit(1);
    }

    // Tokenize the productsList into the products array
    int index = 0;
    char *token = strtok(productsList, "\n");
    while (token != NULL) {
        products[index++] = token;
        token = strtok(NULL, "\n");
    }

    int selected = 0;
    char key;
    int productsNum = get_size(basket);
    int current_page = 0;
    int total_pages = (num_products + page_size - 1) / page_size;

    enable_raw_mode();

    do {
        system("clear");
        // Display menu
        printf("Choose a product:\n");
        int start_index = current_page * page_size;
        int end_index = start_index + page_size;
        if (end_index > num_products) {
            end_index = num_products;
        }
        for (int i = start_index; i < end_index; i++) {
            printf("%s%c%s %s\n", selected == i ? GREEN_COLOR : "", 
                   selected == i ? '>' : ' ', RESET_COLOR, products[i]);
        }

        printf("Page %d/%d\n", current_page + 1, total_pages);
        printf("\nUse arrow up/down keys to select, Left/Right keys to change page, Enter key to add product to cart, Q to finish.\n");


        // Get user input
        key = getch();
        // If ESC
        if (key == 27) {
            getch(); // skip the [
            key = getch(); // get the actual key code
        }

        // Update selection based on input
        switch (key) {
            case 113: // q
                system("clear");
                fflush(stdout);
                if(productsNum > 0) {
                    char final_basket[BUFFER_SIZE] = {0};
                    while(!is_empty(basket)) {
                        strcat(final_basket, ((char*)dequeue(basket)));
                        if(!is_empty(basket)) {
                            strcat(final_basket, ", ");
                        }
                    }
                    strcat(final_basket, "\0");
                    printf("%d products in basket: [%s]\n", productsNum, final_basket);
                } else {
                   printf("%d products in basket.\n", productsNum);
                }
                usleep(1500000);
                free(products); // Free allocated memory
                if (productsNum > 0) {
                    printf("Waiting in queue to pay...\n");
                    usleep(1500000);
                } else {
                    printf("Going to the exit...\n");
                    usleep(1500000);
                }
                return productsNum;
            case 65: // up arrow
                selected = (selected == start_index) ? end_index - 1 : selected - 1;
                break;
            case 66: // down arrow
                selected = (selected == end_index - 1) ? start_index : selected + 1;
                break;
            case 67: // right arrow
                if (current_page < total_pages - 1) {
                    current_page++;
                    selected = current_page * page_size;
                }
                break;
            case 68: // left arrow
                if (current_page > 0) {
                    current_page--;
                    selected = current_page * page_size;
                }
                break;
            case 10: // Enter
                enqueue(basket, products[selected]);
                system("clear");
                fflush(stdout);
                printf("\n\nAdded %s to basket.\n\n", products[selected]);
                productsNum = get_size(basket);
                usleep(1500000);
        }
    } while (1);
}

int setupConnection(void) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Set up the address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_ADDRESS, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    return sock;
} 

void goShopping(int sock, int customerId, int cashierIndex) {
    char productsList[BUFFER_SIZE] = {0};
    basket = create_queue();

    int numbytes = recv(sock, &buffer, BUFFER_SIZE, 0);
    if (numbytes < 0) {
        perror("Communication error");
        exit(EXIT_FAILURE);
    }
    if(strstr(buffer, "Welcome") != NULL) {
        system("clear");
        printf("%s", LOGO);
        usleep(1000000);
        printf("Welcome to Miao Market!\n");
        usleep(1500000);
        printf("Your id is: %d\n", customerId);
        usleep(1500000);
    } else {
        printf("%d Waiting for permission to enter.\n", customerId);
        fflush(stdout);
        memset(buffer, 0, BUFFER_SIZE);
        recv(sock, &buffer, BUFFER_SIZE, 0);
        system("clear");
        printf("%s", LOGO);
        usleep(1000000);
        printf("Welcome to Miao Market!\n");
        usleep(1500000);
        printf("Your id is: %d\n", customerId);
        usleep(1500000);
        
    }
    memset(buffer, 0, BUFFER_SIZE);

    if(recv(sock, &productsList, BUFFER_SIZE, 0) < 0) {
        perror("Error receiving products");
        close(sock);
        exit(EXIT_FAILURE);
    }
    usleep(20000);
    int products = choose_product(productsList, 10);

    // Send number of products and customerId to the server
    send(sock, &products, sizeof(products), 0);
    send(sock, &customerId, sizeof(customerId), 0);
    fflush(stdout);
    recv(sock, &buffer, BUFFER_SIZE, 0);

    if(strstr(buffer, "Payment received") != NULL) {
        printf("Payment confirmed! %d Leaving...\n", customerId);
        usleep(1500000);
    } else if(strstr(buffer, "You can leave!") != NULL) {
        printf("Permission granted from supervisor! %d Leaving...\n", customerId);
        usleep(1500000);
    }

    close(sock);
    destroy_queue(basket);
    disable_raw_mode();
}

// Function to simulate shopping and communicate with the server
void simulateShopping(int sock, int customerId, int shoppingTime, int cashierIndex, int products) {
    char productsList[BUFFER_SIZE] = {0};

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
    memset(buffer, 0, BUFFER_SIZE);

    if(recv(sock, &productsList, BUFFER_SIZE, 0) < 0) {
        perror("Error receiving products");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Received products list, Time for shopping!\n");
    printf("Customer %d that took %d products is shopping for %d seconds.\n", customerId, products, shoppingTime);
    sleep(shoppingTime); // Simulate shopping time

    // Send number of products and customerId to the server
    send(sock, &products, sizeof(products), 0);
    send(sock, &customerId, sizeof(customerId), 0);

    recv(sock, &buffer, BUFFER_SIZE, 0);

    if(strstr(buffer, "Payment received") != NULL) {
        printf("Payment confirmed! %d Leaving...\n", customerId);
    } else if(strstr(buffer, "You can leave!") != NULL) {
        printf("Permission granted from supervisor! %d Leaving...\n", customerId);
    }

    // Close the socket
    close(sock);
}

int main(int argc, char *argv[]) {
    int interactive = 0;
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        interactive = 1;
    }
    srand(time(NULL)+getpid()); // Seed the random number generator
    int customerId = rand() % 1000; // Generate a random customer ID
    int shoppingTime = rand() % 5 + 1; // Generate a random shopping time
    int cashierIndex = rand() % ACTIVE_CASHIERS; // Choose a random cashier index (assuming 3 cashiers)
    int products = rand() % 10; // Generate a random number of products

    memset(buffer, 0, BUFFER_SIZE);
    int socket = setupConnection();

    if(interactive) {
        goShopping(socket, customerId, cashierIndex);
    } else {
        simulateShopping(socket, customerId, shoppingTime, cashierIndex, products);
    }

    return 0;
}
