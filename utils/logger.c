#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Destroy the mutex
void destroy_logger(void) {
    pthread_mutex_destroy(&log_mutex);
}

// Log message function
void log_message(const char *message) {
    // Lock the mutex
    pthread_mutex_lock(&log_mutex);

    // Open the file in append mode
    FILE *file = fopen("./logs/log.txt", "a");
    if (file == NULL) {
        printf("Error opening file!\n");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // Write the message to the file and flush it
    fprintf(file, "%s\n", message);
    fflush(file);
    fclose(file);

    pthread_mutex_unlock(&log_mutex);
}
