#include <stdio.h>
#include <string.h>

#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

char* get_action(char* argv[]);

char* get_remote_file_name(int argc, char* argv[], char* action);

char* get_local_file_name(int argc, char* argv[], char* action);

unsigned char* read_local_file(char* file_name);

char* generate_client_message(int argc, char* argv[], char* action);

size_t get_binary_file_size(char* file_name);

void get_command_handler(char* local_file_name, char* remote_file_name, int argc, 
                        unsigned char* binary_message_received, int bytes_received);

char* event_handler(char* action, char* local_file_name, unsigned char* binary_message, char* remote_file_name, int argc, int bytes_received);

#endif