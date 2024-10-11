#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>    // For threads
#include <signal.h>     // For signal handling

// Buffer size for communication
#define BUFFER_SIZE 1024

// FIFO file paths
extern const char *const input_fifo;
extern const char *const output_fifo;

// Function declarations
// SIGINT handler
void __attribute__((noreturn)) sigint_handler(int signum);
// Uppercase transformation
char upper_filter(char c);
// Lowercase transformation
char lower_filter(char c);
// Null transformation
char null_filter(char c);
// Function pointer for filters
typedef char (*filter_func)(char);
// Function to select the appropriate filter
filter_func select_filter(const char *filter_name);
// Function to handle client in thread
void *handle_client(void *arg);
// Apply the given filter to the string
void apply_filter(char *response, const char *client_string, filter_func filter);

#endif    // SERVER_H
