#define  _GNU_SOURCE

#include "../include/print_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

// Safely lock the mutex and handle errors
static void safe_mutex_lock(pthread_mutex_t *mutex) {
    int err = pthread_mutex_lock(mutex);
    if (err != 0) {
        fprintf(stderr, "Error locking mutex: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

// Safely unlock the mutex and handle errors
static void safe_mutex_unlock(pthread_mutex_t *mutex) {
    int err = pthread_mutex_unlock(mutex);
    if (err != 0) {
        fprintf(stderr, "Error unlocking mutex: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

// Print the progress of each thread
void print_threads(printing_config *data, bool overwrite) {
    safe_mutex_lock(data->mutex); // Lock the mutex to ensure safe output

    for (int i = 0; i < data->num_threads; i++) {
        int progress = (*data->percentages[i]) * data->bar_length / 100;
        printf("\033[2K"); // Clear the current line
        printf("Thread %lu: [%.*s%c%*s] %d%%\n",
                data->threads[i], progress, data->total_bar, data->head_char,
                data->bar_length - progress, "", *data->percentages[i]);
    }
    if(overwrite){
        printf("\033[%dA", data->num_threads); // Move the cursor up by the number of lines corresponding to the number of threads
    }
    fflush(stdout); // Flush the output to ensure it is displayed
    safe_mutex_unlock(data->mutex); // Unlock the mutex
}

// Thread function to print thread progress periodically
void *print_threads_thread(void *args) {
    printing_config *data = (printing_config *)args;

    while (!data->exit_condition) {
        print_threads(data, true); // Print thread progress
        usleep(data->refresh_rate * 1000); // Sleep for the specified refresh rate
    }
    print_threads(data, false); // Final printing
    return NULL;
}

// Start the printing thread with the given configuration
printing_config* print_threads_start(pthread_mutex_t *mutex, pthread_t *threads, int **percentages, int num_threads, int refresh_rate,
                                    int bar_length, char head_char, char body_char) {

    // Allocate memory for the progress bar
    char *total_bar = malloc((bar_length + 1) * sizeof(char));
    if (total_bar == NULL) {
        perror("Error allocating the progress bar");
        exit(EXIT_FAILURE);
    }

    // Fill the progress bar with the body character
    for (int i = 0; i < bar_length; i++) {
        total_bar[i] = body_char;
    }
    total_bar[bar_length] = '\0';

    // Allocate memory for the configuration struct
    printing_config *conf = malloc(sizeof(printing_config));
    if (conf == NULL) {
        perror("Error allocating the configuration");
        free(total_bar);
        exit(EXIT_FAILURE);
    }

    *conf = (printing_config){0, mutex, threads, percentages, num_threads, refresh_rate, bar_length, head_char, total_bar, false};

    if (pthread_create(&conf->print_thread, NULL, &print_threads_thread, conf) != 0) {
        perror("Error creating the printing thread");
        free(total_bar);
        free(conf);
        exit(EXIT_FAILURE);
    }
    return conf;
}

// Stop the printing thread and clean up resources
void print_threads_finish(printing_config *conf) {
    if (conf != NULL) {
        conf->exit_condition = true;
        pthread_join(conf->print_thread, NULL); // join the printing thread
        if (conf->total_bar != NULL) {
            free(conf->total_bar); // Free the progress bar
            conf->total_bar = NULL;
        }
        free(conf); // Free the configuration struct
    }
}

// Print a formatted message within a locked mutex
void print_in_thread(pthread_mutex_t *mutex, const char *format, ...) {
    va_list args;
    va_start(args, format);
    safe_mutex_lock(mutex); // Lock the mutex to ensure safe printing
    printf("\033[2K"); // Clear the current line
    vprintf(format, args); // Print the formatted message
    printf("\n");
    safe_mutex_unlock(mutex); // Unlock the mutex
    va_end(args);
}
