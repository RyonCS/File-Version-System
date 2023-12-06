#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "client_functions.h"

#define MAX_BINARY_SIZE 20 * 1024 * 1024

int main(int argc, char* argv[])
{
  int socket_desc;
  struct sockaddr_in server_addr;
  char server_message[2000];
  char ls_message_received[2000];
  
  // Clean buffers:
  memset(server_message,'\0',sizeof(server_message));
  
  // Create socket:
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  
  if(socket_desc < 0){
    printf("Unable to create socket\n");
    return -1;
  }
  
  printf("Socket created successfully\n");
  
  // Set port and IP the same as server-side:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(2000);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  
  // Send connection request to server:
  if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
    printf("Unable to connect\n");
    return -1;
  }
  printf("Connected with server successfully\n");

  // Create the client message "ACTION|REMOTE FILE NAME|LOCAL FILE CONTENTS";
  char* action = get_action(argv);
  char* local_file_name = get_local_file_name(argc, argv, action);
  char* remote_file_name = get_remote_file_name(argc, argv, action);
  char* client_message = generate_client_message(argc, argv, action);
  printf("Client Message: %s\n", client_message);

  unsigned char* binary_message = NULL;
  size_t binary_message_size = 0;

  if (strcmp(action, "WRITE") == 0) {
    binary_message = read_local_file(local_file_name);
    binary_message_size = get_binary_file_size(local_file_name);

    // Send the message to server:
    if(send(socket_desc, client_message, strlen(client_message), 0) < 0){
      printf("Unable to send client message\n");
      return -1;
    }

    sleep(1);

    if(send(socket_desc, binary_message, binary_message_size, 0) < 0) {
      printf("Unable to send binary message\n");
      return -1;
    }
  } else {
    // Send the message to server:
    if(send(socket_desc, client_message, strlen(client_message), 0) < 0){
      printf("Unable to send client message\n");
      return -1;
    }
  }

  unsigned char* binary_message_received = NULL;
  int bytes_received = 0;

  // Receive message.
  if (strcmp(action, "GET") == 0) {
    int server_bytes_received = recv(socket_desc, server_message, sizeof(server_message) - 1, 0);

    sleep(1);

    if (strcmp(server_message, "File not found.") != 0) {
      binary_message_received = (unsigned char*)malloc(MAX_BINARY_SIZE * sizeof(unsigned char));
      bytes_received = recv(socket_desc, binary_message_received, MAX_BINARY_SIZE, 0);
      get_command_handler(local_file_name, remote_file_name, argc, binary_message_received, bytes_received);
    }
    printf("Server message: %s\n", server_message);

  } else if (strcmp(action, "LS") == 0) {
    int server_bytes_received = recv(socket_desc, server_message, sizeof(server_message) - 1, 0);
    sleep(1);
    int ls_received = recv(socket_desc, ls_message_received, sizeof(ls_message_received) -1, 0);

    printf("Server Message: %s\n", server_message);
    printf("Version History: %s\n", ls_message_received);

  } else {
    int server_bytes_received = recv(socket_desc, server_message, sizeof(server_message) - 1, 0);
    printf("Server Message: %s\n", server_message);
  }

  return 0;
}
