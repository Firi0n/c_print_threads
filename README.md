# :thread: PrintThreads Library

PrintThreads is a C library that helps you print real-time progress bars and formatted messages from multiple threads in a synchronized way.

## :rocket: Features

- Real-time progress bar for each thread.
- Thread-safe printing with mutexes.
- Simple and modular API for easy integration.
- Lightweight and efficient.

---

## :hammer_and_wrench: Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/yourusername/printthreads.git
   cd printthreads
   ```

2. Build the library:

   ```bash
   make
   ```

3. Run the tests:

   ```bash
   make test
   ```

4. Install the library (requires sudo):

   ```bash
   sudo make install
   ```

5. To uninstall:
   ```bash
   sudo make uninstall
   ```

---

## :memo: Usage

To use the library in your C project, include the header file:

```c
#include <print_threads.h>
```

Link the library during compilation:

```bash
gcc -o myprogram myprogram.c -lprintthreads -pthread
```

---

## :bulb: Example

Here is a simple example that demonstrates how to use the PrintThreads library:

**tests/test.c**

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <print_threads.h>

#define NUM_THREADS 2

void* worker(void* arg) {
    int* progress = (int*)arg;
    for (int i = 0; i <= 100; i++) {
        usleep(50000); // Simulate work
        *progress = i;
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int progress[NUM_THREADS] = {0};

    int* progress_ptrs[NUM_THREADS] = {&progress[0], &progress[1]};
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // Start the print thread
    printing_config* config = print_threads_start(&mutex, threads, progress_ptrs, NUM_THREADS, 50, 30, '>', '=');

    // Create worker threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker, (void*)&progress[i]);
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    print_threads_finish(config);
    pthread_mutex_destroy(&mutex);

    return 0;
}
```

To compile and run the test:

```bash
make test
```

---

## :broom: Cleaning Up

To clean up build artifacts, use:

```bash
make clean
```

---

## :file_folder: Project Structure

```
project-root/
├── bin/                # Compiled binaries
├── include/            # Header files
│   └── print_threads.h
├── lib/                # Library source code
│   └── print_threads.c
├── obj/                # Object files
├── tests/              # Test programs
│   └── test.c
├── .gitignore          # Ignored files and folders
├── Makefile            # Build and installation rules
└── README.md           # Project documentation
```

---

## :page_facing_up: License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
