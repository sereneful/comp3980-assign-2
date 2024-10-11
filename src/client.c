#include "client.h"
#include <fcntl.h>    // For O_CLOEXEC and open
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_ARG 5

// FIFO path for sending and receiving data to server
const char *const input_fifo  = "input_fifo";
const char *const output_fifo = "output_fifo";

void print_usage(const char *prog_name)
{
    printf("Usage: %s -s <string> -f <filter>\n", prog_name);
    printf("Options:\n");
    printf("  -s <string>    The string to be transformed\n");
    printf("  -f <filter>    The filter type (upper, lower, null)\n");
}

static int is_filter_valid(const char *filter)
{
    return (strcmp(filter, "upper") == 0 || strcmp(filter, "lower") == 0 || strcmp(filter, "null") == 0);
}

int main(int argc, char *argv[])
{
    int input_fd;
    int output_fd;
    int opt;

    char        buffer[BUFFER_SIZE];
    char        response[BUFFER_SIZE];
    const char *client_string = NULL;
    const char *filter_type   = NULL;

    if(argc > MAX_ARG)
    {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Getopt to parse command line arguments
    while((opt = getopt(argc, argv, "s:f:")) != -1)
    {
        switch(opt)
        {
            case 's':
                client_string = optarg;
                break;
            case 'f':
                filter_type = optarg;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(client_string == NULL || filter_type == NULL)
    {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if(strlen(client_string) == 0)
    {
        printf("Error: The string is empty.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if(!is_filter_valid(filter_type))
    {
        printf("Error: Invalid filter type '%s'.\n", filter_type);
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Concatenate the input string
    snprintf(buffer, sizeof(buffer), "%s:%s", client_string, filter_type);

    // Open the input FIFO and send the data to the server
    input_fd = open(input_fifo, O_WRONLY | O_CLOEXEC);
    if(input_fd == -1)
    {
        perror("Error opening input FIFO");
        exit(EXIT_FAILURE);
    }

    if(write(input_fd, buffer, strlen(buffer) + 1) == -1)
    {
        perror("Error writing to input FIFO");
        close(input_fd);
        exit(EXIT_FAILURE);
    }
    close(input_fd);

    // Open the output FIFO and read the processed response from the server
    output_fd = open(output_fifo, O_RDONLY | O_CLOEXEC);

    if(output_fd == -1)
    {
        perror("Error opening output FIFO");
        exit(EXIT_FAILURE);
    }

    if(read(output_fd, response, sizeof(response)) == -1)
    {
        perror("Error reading from output FIFO");
        close(output_fd);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Processed string: %s\n", response);
    }
    close(output_fd);

    return 0;
}
