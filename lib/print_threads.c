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

#define THREAD_INFO_INITIAL_DIM 10
#define THREAD_INFO_MUL 2

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

void conf_error(printing_config *conf) {
    if (conf == NULL) {
        fprintf(stderr, "[Print threads Configuration Error] Printing configuration is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (conf->mutex == NULL) {
        fprintf(stderr, "[Print threads Configuration Error] Mutex is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (conf->total_bar == NULL) {
        fprintf(stderr, "[Print threads Configuration Error] Total bar is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (conf->threads == NULL) {
        fprintf(stderr, "[Print threads Configuration Error] Threads info array is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (conf->threads_dim < conf->num_threads) {
        fprintf(stderr, "[Print threads Configuration Error] Number of threads (%d) greater than dimension of threads info array (%d)\n",
                conf->num_threads, conf->threads_dim);
        exit(EXIT_FAILURE);
    }
}


printing_config print_threads_init(pthread_mutex_t *mutex, unsigned int refresh_rate, unsigned int bar_length, char head_char, char body_char) {
    if (mutex == NULL)
    {
        fprintf(stderr, "[Print threads Init Error] Mutex is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (refresh_rate == 0)
    {
        fprintf(stderr, "[Print threads Init Error] Refresh rate can't be 0\n");
        exit(EXIT_FAILURE);
    }
    if (bar_length == 0)
    {
        fprintf(stderr, "[Print threads Init Error] Bar length can't be 0\n");
        exit(EXIT_FAILURE);
    }
    
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

    // Allocate memory for the threads information array
    thread_info *threads = malloc(THREAD_INFO_INITIAL_DIM * sizeof(thread_info));
    if (threads == NULL) {
        perror("Error allocating threads information");
        free(total_bar);
        exit(EXIT_FAILURE);
    }

    // Return the initialized printing configuration
    return (printing_config){
        .mutex = mutex,
        .threads = threads,
        .threads_dim = THREAD_INFO_INITIAL_DIM,
        .num_threads = 0,
        .refresh_rate = refresh_rate * 1000,
        .bar_length = bar_length,
        .head_char = head_char,
        .total_bar = total_bar,
        .exit_condition = false
    };
}


void print_threads_add_thread(printing_config *conf, pthread_t thread, unsigned int *percentage) {
    
    conf_error(conf);

    safe_mutex_lock(conf->mutex); // Lock the mutex

    // Check if we need to resize the thread array
    if (conf->num_threads == conf->threads_dim) {
        conf->threads_dim *= 2;
        thread_info *new_threads = realloc(conf->threads, conf->threads_dim * sizeof(thread_info));
        if (new_threads == NULL) {
            perror("Error reallocating threads array");
            safe_mutex_unlock(conf->mutex); // Unlock before exiting
            exit(EXIT_FAILURE);
        }
        conf->threads = new_threads;
    }

    // Add the new thread info
    conf->threads[conf->num_threads] = (thread_info){
        .thread = thread,
        .percentage = percentage,
        .old_percentage = 0
    };
    conf->num_threads++; // Increment the number of threads

    safe_mutex_unlock(conf->mutex); // Unlock the mutex
}

void print_threads_remove_thread(printing_config *conf) {
    
    conf_error(conf);

    safe_mutex_lock(conf->mutex); // Lock the mutex

    if (conf->num_threads == 0) {
        fprintf(stderr, "Error: there aren't threads to remove\n");
    } else {
        conf->num_threads--;
    }
    safe_mutex_unlock(conf->mutex); // Unlock the mutex
}

// Print the progress of a single thread
void print_thread(thread_info *t, unsigned int bar_length, char *total_bar, char head) {
    printf("\033[2K"); // Clear the current line

    // Print the progress from the last percentage to the current one
    for (int i = t->old_percentage; i <= (*t->percentage); i++) {
        int progress = i * bar_length / 100;

        // Print the progress bar with the current percentage
        printf("Thread %lu: [%.*s%c%*s] %d%%\r",
                t->thread, progress, total_bar, head,
                bar_length - progress, "", i);
                fflush(stdout);
    }
    printf("\n");
}

// Print the progress of all threads, optionally overwriting previous output
void print_threads(printing_config *conf, bool overwrite) {
    safe_mutex_lock(conf->mutex); // Lock the mutex to ensure thread safety

    // Print progress for each thread
    for (int i = 0; i < conf->num_threads; i++) {        
        print_thread(&conf->threads[i], conf->bar_length, conf->total_bar, conf->head_char);
    }

    // Move the cursor up to overwrite the previous lines, if needed
    if (overwrite) {
        printf("\033[%dA", conf->num_threads);
    }

    safe_mutex_unlock(conf->mutex); // Unlock the mutex
}

// Thread function that continuously prints thread progress
void *print_threads_thread(void *arg) {
    printing_config *conf = (printing_config*)arg;

    // Continuously print threads' progress while the exit condition is not met
    while (!conf->exit_condition) {
        print_threads(conf, true); // Print progress with overwriting
        usleep(conf->refresh_rate); // Sleep for the specified refresh rate
    }

    // Final print to display the last state without overwriting
    print_threads(conf, false);
    return NULL;
}

// Start the print thread
void print_threads_start(printing_config *conf) {
    conf_error(conf); // Check the configuration for errors
    printf("\e[?25l"); // Hide the cursor
    // Create the print thread and handle potential errors
    int err = pthread_create(&conf->print_thread, NULL, print_threads_thread, (void *)conf);
    if (err != 0) {
        fprintf(stderr, "Error creating print thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

// Stop the printing thread and clean up resources
void print_threads_finish(printing_config *conf) {
    if (conf != NULL) {
        conf->exit_condition = true;
        pthread_join(conf->print_thread, NULL); // Join print thread

        // Free the progress bar
        if (conf->total_bar != NULL) {
            free(conf->total_bar);
            conf->total_bar = NULL;
        }

        // Free the threads information array
        if (conf->threads != NULL) {
            free(conf->threads);
            conf->threads = NULL;
        }

        // Free the configuration struct itself
        free(conf);
    }
    printf("\e[?25h"); // Show the cursor
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
