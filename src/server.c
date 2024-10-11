#include "server.h"
#include <ctype.h>    // For toupper and tolower
#include <fcntl.h>    // For O_CLOEXEC and open
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>    // For mkfifo
#include <time.h>        // For nanosleep
#include <unistd.h>

#define FIFO_PERMISSIONS 0666
#define BUFFER_SIZE 1024
#define SLEEP_SECONDS 0                 // Sleep for 0 seconds
#define SLEEP_NANOSECONDS 500000000L    // Sleep for 500 milliseconds (500,000,000 nanoseconds)

const char *const input_fifo  = "input_fifo";
const char *const output_fifo = "output_fifo";

// Mutex for synchronizing access to the FIFOs
static pthread_mutex_t fifo_mutex = PTHREAD_MUTEX_INITIALIZER;    // NOLINT

// Signal handler SIGINT (graceful shutdown)
void __attribute__((noreturn)) sigint_handler(int signum)
{
    const char *message = "Server shutting down.\n";

    (void)signum;
    unlink(input_fifo);
    unlink(output_fifo);

    write(STDOUT_FILENO, message, strlen(message));

    _exit(0);
}

// Uppercase transformation
char upper_filter(char c)
{
    if(isalpha((unsigned char)c))
    {
        return (char)toupper((unsigned char)c);
    }
    return c;
}

// Lowercase transforamtion
char lower_filter(char c)
{
    if(isalpha((unsigned char)c))
    {
        return (char)tolower((unsigned char)c);
    }
    return c;
}

// Null transformation (no changes)
char null_filter(char c)
{
    return c;
}

// Function pointer type for filter functions
typedef char (*filter_func)(char);

// Select the appropriate filter function
filter_func select_filter(const char *filter_name)
{
    // Uppercase transformation
    if(strcmp(filter_name, "upper") == 0)
    {
        return upper_filter;
    }
    // Lowercase transformation
    if(strcmp(filter_name, "lower") == 0)
    {
        return lower_filter;
    }
    // Null transformation
    return null_filter;
}

// Applying the filter to the entire string
void apply_filter(char *response, const char *client_string, filter_func filter)
{
    for(int i = 0; client_string[i] != '\0'; i++)
    {
        response[i] = filter(client_string[i]);
    }
    // Response is null terminated
    response[strlen(client_string)] = '\0';
}

// Thread function to handle client requests
void *handle_client(void *arg)
{
    int         input_fd;
    int         output_fd;
    char        buffer[BUFFER_SIZE];
    char        response[BUFFER_SIZE];
    const char *client_string;
    const char *filter_name;
    filter_func filter;
    char       *saveptr;
    ssize_t     bytes_read;
    (void)arg;
    // Locking to ensure thread safety when accessing FIFOs
    pthread_mutex_lock(&fifo_mutex);

    input_fd  = open(input_fifo, O_RDONLY | O_CLOEXEC);
    output_fd = open(output_fifo, O_WRONLY | O_CLOEXEC);

    if(input_fd == -1 || output_fd == -1)
    {
        perror("Error opening FIFO in thread");
        pthread_mutex_unlock(&fifo_mutex);
        pthread_exit(NULL);
    }

    bytes_read = read(input_fd, buffer, sizeof(buffer));
    if(bytes_read == -1)
    {
        perror("Error reading from input FIFO");
        close(input_fd);
        close(output_fd);
        pthread_mutex_unlock(&fifo_mutex);
        pthread_exit(NULL);
    }
    buffer[bytes_read] = '\0';    // Null-terminate the string received
    printf("\nServer received data.\n");

    // The input string taken as string and filter
    client_string = strtok_r(buffer, ":", &saveptr);
    filter_name   = strtok_r(NULL, ":", &saveptr);

    if(filter_name == NULL || client_string == NULL)
    {
        printf("Error: Invalid input format. Exiting thread.\n");
        close(input_fd);
        close(output_fd);
        pthread_mutex_unlock(&fifo_mutex);
        pthread_exit(NULL);
    }

    printf("String: %s Filter: %s\n", client_string, filter_name);

    // Select the appropriate filter function
    filter = select_filter(filter_name);

    // Apply the filter to the client's string
    apply_filter(response, client_string, filter);

    printf("Processed response: %s\n", response);

    // Write the processed string back to the client
    if(write(output_fd, response, strlen(response) + 1) == -1)
    {
        perror("Error writing to output FIFO");
        close(input_fd);
        close(output_fd);
        pthread_mutex_unlock(&fifo_mutex);
        pthread_exit(NULL);
    }

    printf("\nResponse sent to client.\n");
    printf("Press Ctrl+C to stop or enter another input.\n");

    close(input_fd);
    close(output_fd);
    pthread_mutex_unlock(&fifo_mutex);
    return NULL;
}

int main(void)
{
    struct timespec sleep_time;
    pthread_t       thread;

    // Create the FIFOs (input and output)
    mkfifo(input_fifo, FIFO_PERMISSIONS);
    mkfifo(output_fifo, FIFO_PERMISSIONS);

    // Sets up the sigint handler
    signal(SIGINT, sigint_handler);

    printf("Server is running. Press Ctrl+C to stop.\n");

    sleep_time.tv_sec  = SLEEP_SECONDS;
    sleep_time.tv_nsec = SLEEP_NANOSECONDS;

    // Main server loop to handle client requests
    while(1)
    {
        pthread_create(&thread, NULL, handle_client, NULL);
        pthread_detach(thread);    // Automatically free resources when done

        nanosleep(&sleep_time, NULL);
    }
}
