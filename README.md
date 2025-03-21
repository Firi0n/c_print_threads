# Print Threads Library 🧵

## Overview

The **Print Threads Library** is a C library designed to facilitate thread-safe printing and progress tracking in
multi-threaded applications. It provides an easy-to-use interface for managing thread progress and output, ensuring
consistency and thread safety.

## Features ✨

-   Dynamically add and remove threads during execution
-   Thread-safe printing using mutexes
-   Real-time progress tracking for multiple threads
-   Customizable progress bars (length, head, and body characters)
-   Automatic resizing of the threads array
-   Graceful shutdown of printing threads

## Getting Started 🚀

### Prerequisites

-   GCC
-   POSIX Threads Library (pthread)

### Building

To compile the library and the test program, use the provided Makefile:

```bash
make
```

This will generate the executable `test`.

#### Makefile Options

The Makefile provides the following options:

-   `make`: Compiles the project and generates the `test` executable and the static library `libprintthreads.a`.
-   `make test`: Runs the compiled test program.
-   `make install`: Installs the library and header to system directories (default prefix: `/usr/local`).
-   `make uninstall`: Removes the installed library and header from system directories.
-   `make clean`: Removes all build artifacts and binaries to clean the build environment.

### Running the Example

To run the example program:

```bash
./test
```

## Library Usage 📚

### Including the Library

Include the library header in your project:

```c
#include "print_threads.h"
```

### Printing Configuration

The core of the library revolves around the `printing_config` struct, which holds information about the printing
settings and tracked threads.

#### Initializing the Configuration

To create a new printing configuration, use the `print_threads_init` function:

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
printing_config conf = print_threads_init(&mutex, 1, 50, '>', '=');
```

-   `mutex`: A pointer to a `pthread_mutex_t` for thread synchronization.
-   `refresh_rate`: The rate at which the progress is updated (in milliseconds).
-   `bar_length`: The length of the progress bar.
-   `head_char`: The character representing the progress head.
-   `body_char`: The character representing the body of the progress bar.

### Threads management

The library manages threads using a `LIFO` (Last In, First Out) stack approach. This means that the most recently added
thread is the first to be removed.

#### Adding Threads

You can dynamically add threads to the printing configuration using:

```c
print_threads_add_thread(&conf, thread_id, &progress);
```

-   `conf`: The printing configuration object.
-   `thread_id`: The ID of the thread to track.
-   `progress`: A pointer to an integer holding the progress percentage (0-100).

#### Removing Threads

To remove a thread when it is no longer needed:

```c
print_threads_remove_thread(&conf);
```

### Starting and Stopping the Print Thread

Start the printing thread to continuously display progress:

```c
print_threads_start(&conf);
```

Stop the printing thread and clean up resources:

```c
print_threads_finish(&conf);
```

### Safe Printing from Threads

The library provides a function to print messages from any thread safely:

```c
print_in_thread(&mutex, "Message: %d", value);
```

This function ensures that the output is not disrupted by concurrent printing.

## Complete Example 💡

The example shown here is a minimal demonstration of the library's usage:

```c
#include "print_threads.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void *worker(void *arg) {
    unsigned int *progress = (unsigned int *)arg;
    for (int i = 0; i <= 100; i++) {
        *progress = i;
        usleep(50000);
    }
    return NULL;
}

int main() {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    printing_config conf = print_threads_init(&mutex, 100, 30, '>', '-');
    pthread_t thread;
    unsigned int progress = 0;

    pthread_create(&thread, NULL, worker, &progress);
    print_threads_add_thread(&conf, thread, &progress);
    print_threads_start(&conf);

    pthread_join(thread, NULL);
    print_threads_finish(&conf);
    return 0;
}
```

For a more comprehensive and practical use case, check out the [test.c](./tests/test.c) file included in the project.

## License 📜

This project is licensed under the MIT License. You can find the full text of the license [here](./LICENSE)📄.
