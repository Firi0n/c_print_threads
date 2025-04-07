#ifndef PRINT_THREADS_H
#define PRINT_THREADS_H

#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>

// Struct to save thread information
typedef struct {
    pthread_t thread;            // Thread identifier
    unsigned short *percentage;    // Pointer to the percentage of thread completion
    unsigned short old_percentage; // Previous percentage of thread completion
} thread_info;

// Struct to save terminal information
typedef struct {
    pthread_t thread;           // Thread responsible for getting terminal width
    pthread_cond_t condition;   // Condition variable for synchronization
    bool sigwinch_received;
    unsigned short width;         // Width of the terminal
    unsigned short height;        // Height of the terminal
} terminal_info;

// Struct to hold configuration for the printing thread progress
typedef struct {
    pthread_t print_thread;                 // Thread responsible for printing progress
    pthread_mutex_t *mutex;                 // Mutex for synchronized output
    terminal_info terminal;                 // Information about terminal
    thread_info *threads;                   // Array of thread information
    unsigned short threads_dim;             // Dimension of the thread information array
    unsigned short num_threads;             // Number of threads being tracked
    unsigned int refresh_rate;              // Refresh rate of the printing (in milliseconds)
    char head_char;                         // Character used as the head of the progress bar
    char body_char;                         // Character used as the body of the progress bar
    char *total_bar;                        // Complete bar representation
    bool exit_condition;                    // Condition to stop the printing thread
} printing_config;


/**
 * @brief Initialize the printing configuration.
 * 
 * @param mutex The mutex to synchronize printing.
 * @param refresh_rate The refresh rate of the printing (in milliseconds).
 * @param head_char The character used as the head of the progress bar.
 * @param body_char The character used as the body of the progress bar.
 * @return A printing_config struct initialized with the given parameters.
 */
printing_config print_threads_init(pthread_mutex_t *mutex, unsigned int refresh_rate, char head_char, char body_char);

/**
 * @brief Add a thread to the printing configuration.
 * 
 * @param thread The thread identifier to be tracked.
 * @param percentage Pointer to the percentage variable of the thread.
 */
void print_threads_add_thread(pthread_t thread, unsigned short *percentage);

/**
 * @brief Remove a thread from the printing configuration.
 */
void print_threads_remove_thread();

/**
 * @brief Start the printing thread to continuously display progress.
 * 
 * @param conf The printing configuration containing the threads to be monitored.
 */
void print_threads_start(printing_config *conf);

/**
 * @brief Stop the printing thread and clean up resources.
 */
void print_threads_finish();

/**
 * @brief Print a formatted message safely from a thread.
 * 
 * @param mutex The mutex to lock during printing.
 * @param format The format string (like in printf).
 * @param ... Additional arguments for the format string.
 */
void print_in_thread(pthread_mutex_t *mutex, const char *format, ...);

#endif // PRINT_THREADS_H
