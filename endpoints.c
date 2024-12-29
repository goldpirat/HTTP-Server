#include "message_handle_tools.h"

// Helper function to remove curly braces from JSON data
char *clean_json(const char *json_data) { // Use const for input
    size_t json_length = strlen(json_data);
    char *cleaned_json = (char *)malloc(json_length + 1); // Allocate sufficient memory
    if (cleaned_json == NULL) {
        perror("malloc failed");
        return NULL;
    }

    size_t cleaned_length = 0;
    for (size_t i = 0; i < json_length; i++) {
        if (json_data[i] != '{' && json_data[i] != '}') {
            cleaned_json[cleaned_length++] = json_data[i];
        }
    }
    cleaned_json[cleaned_length] = '\0';

    return cleaned_json;
}

char *create_user(const char *data) { // Use const for input
    FILE *user_file = fopen("server_resources/users.txt", "a");
    if (user_file == NULL) {
        perror("fopen failed");
        return NULL;
    }

    char *cleaned_data = clean_json(data);
    if (cleaned_data == NULL) {
        fclose(user_file);
        return NULL;
    }

    char *username = parse_key_value(cleaned_data, "\"username\"", ",", ':');
    char *password = parse_key_value(cleaned_data, "\"password\"", ",", ':');

    free(cleaned_data);

    if (username == NULL || password == NULL) {
        fclose(user_file);
        free(username); // Free even if NULL
        free(password); // Free even if NULL
        return NULL;
    }

    fprintf(user_file, "[ %s : %s ]\n", username, password);
    fclose(user_file);

    // Dynamically allocate memory for the response body
    char *response_body = (char *)malloc(256);
    if (response_body == NULL) {
        perror("malloc failed");
        free(username);
        free(password);
        return NULL;
    }
    snprintf(response_body, 256, "{\"message\": \"User created successfully\", \"username\": %s}", username);

    free(username);
    free(password);
    return response_body;
}
