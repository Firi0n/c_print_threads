#ifndef PRINT_THREADS_H
#define PRINT_THREADS_H

#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>

// Struct to hold configuration for printing thread progress
typedef struct {
    pthread_t print_thread;     // Thread responsible for printing progress
    pthread_mutex_t *mutex;     // Mutex for synchronized output
    pthread_t *threads;         // Array of threads to monitor
    int **percentages;          // Array of pointers to thread progress percentages
    int num_threads;            // Number of threads being tracked
    int refresh_rate;           // Refresh rate of the printing (in milliseconds)
    int bar_length;             // Length of the progress bar
    char head_char;             // Character used as the head of the progress bar
    char *total_bar;            // Complete bar representation
    bool exit_condition;
} printing_config;

/**
 * Start the printing thread to continuously display the progress of other threads
 * 
 * @param mutex Pointer to the mutex used for synchronizing output
 * @param threads Array of thread IDs to monitor
 * @param percentages Array of pointers to integers representing the progress of each thread
 * @param num_threads Number of threads to be tracked
 * @param refresh_rate Update rate of the printing (in milliseconds)
 * @param bar_length Length of the progress bar
 * @param head_char Character used as the moving head of the progress bar
 * @param body_char Character used as the body of the progress bar
 * @return Pointer to the printing configuration structure
 */
printing_config* print_threads_start(pthread_mutex_t *mutex, pthread_t *threads, int **percentages, int num_threads, int refresh_rate,
                                    int bar_length, char head_char, char body_char);

/**
 * Terminate the printing thread and clean up allocated resources
 * 
 * @param conf Pointer to the printing configuration structure to be cleaned up
 */
void print_threads_finish(printing_config *conf);

/**
 * Print a formatted message from within a thread, ensuring synchronization using a mutex
 * 
 * @param mutex Pointer to the mutex used to protect the print operation
 * @param format Format string (like printf)
 * @param ... Additional arguments to format the string
 */
void print_in_thread(pthread_mutex_t *mutex, const char *format, ...);

#endif // PRINT_THREADS_H

