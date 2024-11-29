#include "hook.h"

#include "../logmsg.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"


CURL *curl = NULL;

// read config file
int hook_read_config(const char *filename, char **url, char **token) {
    // Open JSON file
    FILE *file = fopen(filename, "r");
    if (!file) {
        merror("Failed to open file %s: %s", filename, strerror(errno));
        return 1;
    }

    // Read file content
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    if (!data) {
        fclose(file);
        return 2;
    }
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    // Parse JSON
    cJSON *json = cJSON_Parse(data);
    free(data);
    if (!json) {
        return 3;
    }

    // Extract URL
    cJSON *json_url = cJSON_GetObjectItemCaseSensitive(json, "url");
    if (cJSON_IsString(json_url) && (json_url->valuestring != NULL)) {
        *url = strdup(json_url->valuestring);
    } else {
        cJSON_Delete(json);
        return 4;
    }

    // Extract Token
    cJSON *json_token = cJSON_GetObjectItemCaseSensitive(json, "token");
    if (cJSON_IsString(json_token) && (json_token->valuestring != NULL)) {
        *token = strdup(json_token->valuestring);
    } else {
        cJSON_Delete(json);
        return 5;
    }

    cJSON_Delete(json);  // Free JSON structure
    return 0;
}


// Initialize curl globally
int hook_init() {
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        return 1;  // Failure
    }
    curl = curl_easy_init();
    return (curl != NULL) ? 0 : 1;
}

// Callback function to write response data
size_t hook_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL) return 0;  // Out of memory

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

// Function to make GET or POST requests with dynamic JSON data in POST
int hook_request(const char *url, const char *method, const char *token, const char *text, char **response) {
    CURL *curl = curl_easy_init();
    CURLcode res;
    struct memory buffer = {0};  // Initialize buffer for response

    if (!curl) {
        merror("CURL not initialized\n");
        return 1;  // Error 1: CURL initialization failed
    }

    // Escape double quotes in the input text
    size_t text_length = strlen(text);
    size_t escaped_size = 2 * text_length + 1;  // Maximum size after escaping
    char *escaped_text = malloc(escaped_size);
    if (!escaped_text) {
        merror("Failed to allocate memory for escaped text\n");
        curl_easy_cleanup(curl);
        return 2;  // Error 2: Memory allocation for escaped text failed
    }

    char *dest = escaped_text;
    for (const char *src = text; *src; src++) {
        if (*src == '"') {
            *dest++ = '\\';  // Escape double quotes
        }
        *dest++ = *src;
    }
    *dest = '\0';  // Null-terminate the escaped string

    // Calculate the size needed for the JSON payload
    size_t json_size = strlen(escaped_text) + 20;  // 20 extra for '{"text": "', '}', and null terminator

    // Allocate memory for the JSON payload
    char *json_data = malloc(json_size);
    if (!json_data) {
        merror("Failed to allocate memory for JSON data\n");
        free(escaped_text);  // Free the allocated escaped text
        curl_easy_cleanup(curl);
        return 3;  // Error 3: Memory allocation for JSON data failed
    }

    // Create JSON payload from escaped text input
    snprintf(json_data, json_size, "{\"text\": \"%s\"}", escaped_text);
    free(escaped_text);  // Free the temporary escaped text

    mwarn("JSON Data: %s", json_data);

    // Set up the request URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, hook_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);

    // Set HTTP method and data
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);  // Set JSON data for POST request
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    // Set headers, including token if provided
    struct curl_slist *headers = NULL;
    if (token) {
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);
        headers = curl_slist_append(headers, auth_header);
    }
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
        merror("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_slist_free_all(headers);
        free(buffer.response);
        free(json_data);  // Free allocated JSON data
        curl_easy_cleanup(curl);
        return 4;  // Error 4: curl_easy_perform failed
    }

    // Set response and clean up
    *response = buffer.response;  // Pass response to the caller
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_data);  // Free allocated JSON data after the request

    return 0;  // Success
}

// Cleanup function
void hook_close() {
    if (curl) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
}


