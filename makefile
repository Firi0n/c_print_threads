# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -g -O -pthread
LDLIBS = -lm -pthread
PREFIX = /usr/local

# Target library and object files
TARGET = libprintthreads.a
OBJ = print_threads

# Default build target
all: bin/libprintthreads.a

# Compile object file from source
obj/$(OBJ).o: lib/$(OBJ).c include/$(OBJ).h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c lib/$(OBJ).c -o obj/$(OBJ).o

# Create the static library archive
bin/libprintthreads.a: obj/$(OBJ).o
	@mkdir -p bin
	ar rcs bin/$(TARGET) obj/$(OBJ).o

# Compile the test program
bin/test: tests/test.c bin/libprintthreads.a
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test tests/test.c -Lbin -lprintthreads $(LDLIBS) -Iinclude

# Run the test program
test: bin/test
	./bin/test

# Install the library and header to system directories
install: bin/libprintthreads.a
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp bin/libprintthreads.a $(PREFIX)/lib
	cp include/print_threads.h $(PREFIX)/include

# Uninstall the library and header from system directories
uninstall:
	rm -f $(PREFIX)/lib/libprintthreads.a
	rm -f $(PREFIX)/include/print_threads.h

# Clean up all build artifacts and binaries
clean:
	rm -rf obj/ bin/
