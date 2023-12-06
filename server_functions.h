#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server_result_struct.h"

#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

char* get_action(char* message);

char* get_remote_file_path(char* message);

char* get_file_name(char* file_path);

char* get_directory_path(char* file_path);

void create_root_directory(char* cwd);

void create_directories(char* cwd, char* directory_path);

void write_file(char* cwd, char* directory_path, char* file_name, char* message, int binary_bytes_received, server_result_t* server_result);

char* get_current_time();

int get_version_number(char* file_path);

void write_command_handler(char* cwd, char* directory_path, char* file_name, unsigned char* file_contents, int binary_bytes_received,
                            server_result_t* server_result);

char* get_most_recent_file_info(char* version_file);

server_result_t* get_command_handler(char* cwd, char* directory_path, char* file_name, server_result_t* server_result);

int remove_command_handler(char* cwd, char* directory_path, char* file_name, server_result_t* server_result);

size_t get_binary_file_size(char* file_path);

void file_version_handler(char* cwd, char* directory_path, char* file_name, server_result_t* server_result);

server_result_t* event_handler(char* action, char* cwd, char* directory_path, char* file_name, 
unsigned char* received_binary_data, int binary_bytes_received, server_result_t* server_result);

void* client_handler(void* socket_desc);

#endif