#ifndef CLIENT_H
#define CLIENT_H

// Buffer size for communication
#define BUFFER_SIZE 1024

// FIFO file paths
extern const char *const input_fifo;
extern const char *const output_fifo;

// Function for usage instructions
void print_usage(const char *prog_name);
// Function for filter validation
static int is_filter_valid(const char *filter);

#endif    // CLIENT_H
