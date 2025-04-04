#define  _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/print_threads.h"

#define NUM_THREADS 5

// Structure to hold thread arguments
typedef struct {
    pthread_mutex_t *mutex;  // Mutex for synchronization
    unsigned int id;                  // Thread ID
    unsigned int max_count;           // Maximum count to reach
    unsigned int delay_us;            // Delay in microseconds
    unsigned int progress;            // Progress percentage
} thread_args;

// Function executed by each thread
void *worker_thread(void *arg) {
    thread_args *args = (thread_args *)arg;
    for (int i = 0; i <= args->max_count; i++) {
        pthread_mutex_lock(args->mutex);
        args->progress = (int)((float)i * 100 / args->max_count);
        pthread_mutex_unlock(args->mutex);
        print_in_thread("Thread %d: %d", args->id, i);
        usleep(args->delay_us*1000);
    }
    print_in_thread("Thread %d finished!", args->id);
    return NULL;
}

// Main function
int main(int argc, char *argv[]) {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t threads[NUM_THREADS];
    thread_args args[NUM_THREADS];
    printing_config print_conf = print_threads_init(&mutex, 1, '>', '=');

    print_threads_start(&print_conf);

    // Initialize and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = (thread_args){&mutex, i, 100, 50, 0};
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
        print_threads_add_thread(threads[i], &args[i].progress);
    }

    // Wait for threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Cleanup and exit
    print_threads_finish();
    return 0;
}

