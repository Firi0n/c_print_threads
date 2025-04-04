#define  _GNU_SOURCE

#include "../include/print_threads.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#define THREAD_INFO_INITIAL_DIM 10
#define THREAD_INFO_MUL 2

#define ERROR(type) "[Print threads: " type " error] "

#define MUTEX_ERROR ERROR("Mutex")
#define CONFIGURATION_ERROR ERROR("Configuration")
#define INIT_ERROR ERROR("Init")
#define THREAD_ERROR ERROR("Thread")
#define START_ERROR ERROR("Start")
#define PRINT_ERROR ERROR("Print")

// Safely lock the mutex and handle errors
static void safe_mutex_lock(pthread_mutex_t *mutex) {
    int err = pthread_mutex_lock(mutex);
    if (err != 0) {
        fprintf(stderr, MUTEX_ERROR "Error locking mutex: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

// Safely unlock the mutex and handle errors
static void safe_mutex_unlock(pthread_mutex_t *mutex) {
    int err = pthread_mutex_unlock(mutex);
    if (err != 0) {
        fprintf(stderr, MUTEX_ERROR "Error unlocking mutex: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

// Controls for configuration errors
void conf_error(printing_config *conf) {
    if (conf == NULL) {
        fprintf(stderr, CONFIGURATION_ERROR "Configuration is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (conf->mutex == NULL) {
        fprintf(stderr, CONFIGURATION_ERROR "Mutex is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (conf->threads == NULL) {
        fprintf(stderr, CONFIGURATION_ERROR "Threads info array is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (conf->threads_dim < conf->num_threads) {
        fprintf(stderr, CONFIGURATION_ERROR "Number of threads (%d) greater than dimension of threads info array (%d)\n",
                conf->num_threads, conf->threads_dim);
        exit(EXIT_FAILURE);
    }
}

// Initialize the printing_config struct
printing_config print_threads_init(pthread_mutex_t *mutex, unsigned int refresh_rate, char head_char, char body_char) {
    if (mutex == NULL)
    {
        fprintf(stderr, INIT_ERROR "Mutex is NULL\n");
        exit(EXIT_FAILURE);
    }
    if (refresh_rate == 0)
    {
        fprintf(stderr, INIT_ERROR "Refresh rate can't be 0\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the threads information array
    thread_info *threads = malloc(THREAD_INFO_INITIAL_DIM * sizeof(thread_info));
    if (threads == NULL) {
        perror(INIT_ERROR "Error allocating threads information");
        exit(EXIT_FAILURE);
    }

    // Return the initialized printing configuration
    return (printing_config){
        .mutex = mutex,
        .threads = threads,
        .threads_dim = THREAD_INFO_INITIAL_DIM,
        .num_threads = 0,
        .refresh_rate = refresh_rate * 1000,
        .terminal = {
            .width = 0,
            .old_width = 0
        },
        .head_char = head_char,
        .body_char = body_char,
        .total_bar = NULL,
        .exit_condition = false
    };
}

// Pushes a thread onto the LIFO
void print_threads_add_thread(printing_config *conf, pthread_t thread, unsigned int *percentage) {
    
    conf_error(conf);

    safe_mutex_lock(conf->mutex); // Lock the mutex

    // Check if we need to resize the thread array
    if (conf->num_threads == conf->threads_dim) {
        conf->threads_dim *= 2;
        thread_info *new_threads = realloc(conf->threads, conf->threads_dim * sizeof(thread_info));
        if (new_threads == NULL) {
            perror(THREAD_ERROR "Error reallocating threads array");
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

// Pops a thread from the LIFO
void print_threads_remove_thread(printing_config *conf) {
    
    conf_error(conf);

    safe_mutex_lock(conf->mutex); // Lock the mutex

    if (conf->num_threads == 0) {
        fprintf(stderr, THREAD_ERROR "There aren't threads to remove\n");
    } else {
        conf->num_threads--;
    }
    safe_mutex_unlock(conf->mutex); // Unlock the mutex
}

// Return the width of the terminal
unsigned int get_terminal_width(){
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        fprintf(stderr, CONFIGURATION_ERROR "Error getting terminal size: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return w.ws_col;
}

//Thread for get terminal width
void *get_terminal_width_thread(void *arg){
    printing_config *conf = (printing_config*)arg;
    while (!conf->exit_condition)
    {
        unsigned int new_width = get_terminal_width();
        // Compare the widths and change them only if necessary.
        if (conf->terminal.width != new_width)
        {
            safe_mutex_lock(conf->mutex);
            // Redefine total bar
            char *temp_total_bar = realloc(conf->total_bar, (new_width+1)*sizeof(char));
            if (temp_total_bar == NULL) {
                fprintf(stderr, CONFIGURATION_ERROR "Error memory allocation: %s\n", strerror(errno));
                safe_mutex_unlock(conf->mutex);
                continue;
            }
            conf->total_bar = temp_total_bar;
            memset(conf->total_bar, conf->body_char, new_width);
            conf->total_bar[new_width] = '\0';
            // Terminal width update
            conf->terminal.old_width = conf->terminal.width;
            conf->terminal.width = new_width;

            safe_mutex_unlock(conf->mutex);
        }
        usleep(conf->refresh_rate);
    }
    return NULL;
}

// Print the progress of a single thread
void print_thread(thread_info *t, unsigned int terminal_width, char *total_bar, char head) {
    printf("\033[2K"); // Clear the current line

    unsigned int bar_length = terminal_width - snprintf(NULL, 0, "Thread %lu: [h] 100%%", t->thread) - 5;

    // Print the progress from the last percentage to the current one
    for (int i = t->old_percentage; i <= (*t->percentage); i++) {
        int progress = i * bar_length / 100;

        // Print the progress bar with the current percentage
        printf("Thread %lu: [%.*s%c%*s] %*d%%\r",
                t->thread, progress, total_bar, head,
                bar_length - progress, "", 3, i);
    }
    t->old_percentage = (*t->percentage); // Update old percentage
    printf("\n");
}

// Print the progress of all threads, optionally overwriting previous output
void print_threads(printing_config *conf, bool overwrite) {
    safe_mutex_lock(conf->mutex); // Lock the mutex to ensure thread safety

    // Print progress for each thread
    for (int i = 0; i < conf->num_threads; i++) {        
        print_thread(&conf->threads[i], conf->terminal.width, conf->total_bar, conf->head_char);
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

    // Create the get terminal width thread and handle potential errors
    int err = pthread_create(&conf->terminal.thread, NULL, get_terminal_width_thread, (void *)conf);
    if (err != 0) {
        fprintf(stderr, START_ERROR "Error creating terminal width thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    // Create the print thread and handle potential errors
    err = pthread_create(&conf->print_thread, NULL, print_threads_thread, (void *)conf);
    if (err != 0) {
        fprintf(stderr, START_ERROR "Error creating print thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

// Stop the printing thread and clean up resources
void print_threads_finish(printing_config *conf) {
    if (conf != NULL) {
        conf->exit_condition = true;
        pthread_join(conf->print_thread, NULL); // Join print thread
        pthread_join(conf->terminal.thread, NULL); // Join get terminal width thread

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
