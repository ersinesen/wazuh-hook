#ifndef _HOOK_H_
#define _HOOK_H_

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct memory {
    char *response;
    size_t size;
};

int hook_init();
int hook_read_config(const char *filename, char **url, char **token);
size_t hook_callback(void *data, size_t size, size_t nmemb, void *userp);
int hook_request(const char *url, const char *method, const char *token, const char *text, char **response);
void hook_close();


#endif
