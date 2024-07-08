#ifndef SERVER_RESULT_H
#define SERVER_RESULT_H

#include <stdio.h>
#include <stdlib.h>

typedef struct server_result {
    unsigned char* data;
    size_t size;
    int success;
    char* file_versions;
    char* message;
} server_result_t;

#endif
