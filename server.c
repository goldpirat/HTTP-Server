#include "message_handle_tools.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_REQUEST_LENGTH 1024  
#define SERVER_PORT 8080       
#define MAX_THREADS 30         

// Structure to hold thread arguments
typedef struct {
    int client_fd;             
    pthread_mutex_t post_mutex; 
} ThreadArgs;


// Thread function to handle client connections
static void *handle_client(void *arg) { 
    ThreadArgs *thread_args = (ThreadArgs *)arg;
    int client_fd = thread_args->client_fd;

    char request[MAX_REQUEST_LENGTH];
    ssize_t bytes_received = recv(client_fd, request, MAX_REQUEST_LENGTH, 0);

    if (bytes_received < 0) {
        perror("recv failed");
        close(client_fd);
        return NULL;
    } else if (bytes_received == 0) {
        printf("Client disconnected.\n");
        close(client_fd);
        return NULL;
    }

    printf("Request received:\n%.*s\n", (int)bytes_received, request); // Print received request

    struct message client_request;
    if (parse_request(&client_request, request, bytes_received) < 0) {
        fprintf(stderr, "Error parsing request.\n");

        // Send 400 Bad Request
        const char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nBad Request";
        send(client_fd, bad_request, strlen(bad_request), 0);

        message_cleanup(&client_request);
        close(client_fd);
        return NULL;
    }

    struct message server_response;
    if (create_response(&client_request, &server_response, &thread_args->post_mutex) < 0) {
        fprintf(stderr, "Error creating response.\n");
        message_cleanup(&client_request);
        close(client_fd);
        return NULL;
    }

    // Construct response string
    size_t response_length = strlen(server_response.line) + strlen(server_response.headers) + strlen(server_response.body) + 6;
    char response[response_length];
    snprintf(response, response_length, "%s\r\n%s\r\n\r\n%s", server_response.line, server_response.headers, server_response.body);

    send(client_fd, response, strlen(response), 0);

    printf("Response sent:\n%s\n", response); // Print sent response

    message_cleanup(&client_request);
    message_cleanup(&server_response);
    close(client_fd);

    return NULL;
}

int main(void) {
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }
    printf("Socket created.\n");

    // Set SO_REUSEADDR option
    int reuse_addr = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Set up server address
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }
    printf("Socket bound to port %d.\n", SERVER_PORT);

    while (1) {
        // Listen for connections
        if (listen(server_fd, MAX_THREADS) < 0) {
            perror("Listen failed");
            close(server_fd);
            return EXIT_FAILURE;
        }

        // Accept connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            close(server_fd);
            return EXIT_FAILURE;
        }
        printf("Connection accepted.\n");

        ThreadArgs thread_args = { .client_fd = client_fd, .post_mutex = PTHREAD_MUTEX_INITIALIZER };

        // Create thread
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, &thread_args) != 0) {
            perror("Thread creation failed");
            close(client_fd);
            continue; // Continue accepting other clients
        }

        // Detach thread
        pthread_detach(thread_id);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
