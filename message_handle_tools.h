#ifndef MESSAGE_HANDLE_TOOLS_H 
#define MESSAGE_HANDLE_TOOLS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef enum { GET, POST, UNSUPPORTED } request_types; 

struct message {
    char *line;
    char *headers;
    char *body;
    request_types type;  // Renamed request_type to type
};

// Function prototypes with const correctness
int parse_request(struct message *req, const char *req_buff, int req_size);
void message_cleanup(struct message *msg);
int create_response(struct message *req, struct message *resp, pthread_mutex_t *POST_lock);

char *parse_key_value(const char *pairs, const char *target, const char *pair_seperator, char key_value_seperator);

// API endpoint function prototypes
char *create_user(const char *data);  // Added const

#endif
