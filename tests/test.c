#define  _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/print_threads.h"

#define NUM_THREADS 5

// Structure to hold thread arguments
typedef struct {
    pthread_mutex_t *mutex;  // Mutex for synchronization
    int id;                  // Thread ID
    int max_count;           // Maximum count to reach
    int delay_us;            // Delay in microseconds
    int progress;            // Progress percentage
} thread_args;

// Function executed by each thread
void *worker_thread(void *arg) {
    thread_args *args = (thread_args *)arg;
    for (int i = 0; i <= args->max_count; i++) {
        pthread_mutex_lock(args->mutex);
        args->progress = (int)((float)i * 100 / args->max_count);
        pthread_mutex_unlock(args->mutex);
        print_in_thread(args->mutex, "Thread %d: %d", args->id, i);
        usleep(args->delay_us*1000);
    }
    print_in_thread(args->mutex, "Thread %d finished!", args->id);
    return NULL;
}

// Main function
int main(int argc, char *argv[]) {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t threads[NUM_THREADS];
    thread_args args[NUM_THREADS];
    int *progress_refs[NUM_THREADS];

    // Initialize and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = (thread_args){&mutex, i, 100, 10, 0};
        progress_refs[i] = &args[i].progress;
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    // Start printing print_configuration
    printing_config *print_conf = print_threads_start(&mutex, threads, progress_refs, NUM_THREADS, 1, 50, '>', '=');

    // Wait for threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Cleanup and exit
    print_threads_finish(print_conf);
    return 0;
}

