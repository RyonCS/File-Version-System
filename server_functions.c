#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "server_result_struct.h"

#define MESSAGE_SIZE 8196
#define MAX_BINARY_SIZE 20 * 1024 * 1024
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Gets the client's action from the client message.
 * 
 * @param message - the message sent from client to server.
 * 
 * @return the action in the client's message.
*/
char* get_action(char* message) {
    char* token = strtok(message, "|");

    if (token != NULL) {
        return token;
    } else {
        return NULL;
    }
}

/**
 * Gets the remote file path from the client's message.
 * 
 * @param message - the message sent from client to server.
 * 
 * @return the remote file path including file name.
*/
char* get_remote_file_path(char* message) {
    char* token = strtok(NULL, "|");
    if (token != NULL) {
        return token;
    } else {
        return NULL;
    }
}

/**
 * Gets the file name from the client's message.
 * 
 * @param file_path - the file path sent from client.
 * 
 * @return the exact name of the file.
*/
char* get_file_name(char* file_path) {
    char* last_item = strrchr(file_path, '/');

    // Path exists, extract file name.
    if (last_item != NULL) {
        return last_item + 1;
    } else { // Only file name provided.
        return file_path;
    }
}

/**
 * Gets the directory portion from the remote file path, excluding file name.
 * 
 * @param file_path - the file path sent from client.
 * 
 * @return the directory path excluding file names. 
*/
char* get_directory_path(char* file_path) {
    const char* file_name = strrchr(file_path, '/');

    if (file_name != NULL) {
        size_t length = file_name - file_path;

        char* directory_path = (char*)malloc((length + 1) * sizeof(char));
        if (directory_path == NULL) {
            printf("ERROR getting directory path.");
            exit(EXIT_FAILURE);
        }

        strncpy(directory_path, file_path, length);
        directory_path[length] = '\0';
        return directory_path;
    } else {
        char* directory_path = (char*)malloc(sizeof(char));
        *directory_path = '\0';
    }
    return NULL;
}

/**
 * Generate a root directory if it's the client's first time using the server.
 * 
 * @param root - the current working directory + /root.
*/
void create_root_directory(char* root) {
    struct stat info;
    if (stat(root, &info) == 0&& S_ISDIR(info.st_mode)) {
        return;
    } else {
        if (mkdir(root, 0777) == 0) {
            printf("Root Directory Created!\n");
        } else {
            perror("Failed to create root directory.\n");
        }
    }
}

/**
 * Create directories if the client's file path included them and they don't already exist.
 * 
 * @param cwd - the current working directory.
 * @param directory_path - the path to the file excluding file name.
*/
void create_directories(char* cwd, char* directory_path) {

    char* token = strtok(directory_path, "/");
    char path[1024];

    strcpy(path, cwd);

    while (token != NULL) {
        strcat(path, "/");
        strcat(path, token);

        struct stat st;
        if(stat(path, &st) == -1) {
            mkdir(path, 0700);
        }

        token = strtok(NULL, "/");
    }
}

/**
 * Gets the current time for file versioning.
 * 
 * @return the current time in a character array.
*/
char* get_current_time() {
    time_t current_time;
    struct tm* time_info;
    static char time_char[20];

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(time_char, sizeof(time_char), "%Y-%m-%d %H:%M:%S", time_info);

    return time_char;
}

/**
 * Generates the version number by reading how many lines are in the file_version file, so we can create a new file version.
 * 
 * @param file_path - the path to the file_version file.
 * 
 * @return the numbers of lines in the file which will indicate the new file's version.
*/
int get_version_number(char* file_path) {
    pthread_mutex_lock(&file_mutex);
    // Open the file_version file and read the number of lines.
    FILE* file = fopen(file_path, "r");
    int line_count = 0;
    char buffer[1024];
    if (file != NULL) {
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            line_count++;
        }

        fclose(file);
        pthread_mutex_unlock(&file_mutex);
        return line_count;
    } else {
        fclose(file);
        pthread_mutex_unlock(&file_mutex);
        return -1;
    }
    pthread_mutex_unlock(&file_mutex);
    return -1;
}

/**
 * Writes binary data to a provided file.
 * 
 * @param cwd - the current working directory.
 * @param directory path - the path to the file excluding file name.
 * @param file_name - the name of the file to write to.
 * @param binary message - the binary data sent from client to server.
 * @param bytes_received - the size of the binary data.
 * @param server_result - a struct to hold data and server messages.
*/
void write_file(char* cwd, char* directory_path, char* file_name, unsigned char* binary_message, int bytes_received,
                server_result_t* server_result) {
    // Generate a path from the cwd to the file.                
    char* token = strtok(directory_path, "/");
    char path[1024];

    strcpy(path, cwd);

    while (token != NULL) {
        strcat(path, "/");
        strcat(path, token);
        token = strtok(NULL, "/");
    }

    strcat(path, "/");
    strcat(path, file_name);

    // Generate a path to the file_version file.
    char version_file_name[1024];
    strcat(version_file_name, path);
    strcat(version_file_name, "_version_history.txt");

    // If file version history doesn't already exist... writing file for first time, create version file.
    if (access(version_file_name, F_OK) == -1) {
        pthread_mutex_lock(&file_mutex);
        // Write binary data to provided file.
        FILE* file = fopen(path, "wb");
        if (file != NULL) {
            fwrite(binary_message, sizeof(unsigned char), bytes_received, file);
            fclose(file);
            pthread_mutex_unlock(&file_mutex);

            // Write version data to file_version file including file name and timestamp in format "file | time".
            pthread_mutex_lock(&file_mutex);
            FILE* version_file = fopen(version_file_name, "w");
            char* file_time = get_current_time();
            strcat(file_name, " ");
            strcat(file_name, file_time);
            fprintf(version_file, "\n%s", file_name);
            fclose(version_file);
            pthread_mutex_unlock(&file_mutex);

            // Add data and server message to struct.
            server_result->success = 1;
            server_result->message = "Message successfully written.";
        } else { // Else, there was an issue opening the file at the given file path.
            fclose(file);
            pthread_mutex_unlock(&file_mutex);
            printf("Error creating file.\n");

            // Add data and server message to struct.
            server_result->success = 0;
            server_result->message = "Failed to write message.";
        }
    } else { // Creating additional version.
        // Get the version number and create a new file name with version number.
        int version_number = get_version_number(version_file_name);
        char version_char[20];
        sprintf(version_char, "%d", version_number);
        strcat(path, version_char);

        pthread_mutex_lock(&file_mutex);
        // Write data to newly named file.
        FILE* file = fopen(path, "wb");
        if (file != NULL) {
            fwrite(binary_message, sizeof(unsigned char), bytes_received, file);
            fclose(file);
            pthread_mutex_unlock(&file_mutex);

            pthread_mutex_lock(&file_mutex);
            // Update the file_version file with this new file's name and date.
            FILE* version_file = fopen(version_file_name, "a");
            char* file_time = get_current_time();
            strcat(file_name, version_char);
            strcat(file_name, " ");
            strcat(file_name, file_time);
            
            fprintf(version_file, "\n%s", file_name);
            fclose(version_file);
            pthread_mutex_unlock(&file_mutex);

            // Add data and server message to struct.
            server_result->success = 1;
            server_result->message = "Message successfully written.";
        } else { // Else there was an error opening the new file version to write.
            fclose(file);
            pthread_mutex_unlock(&file_mutex);
            printf("Error creating file.\n");

            // Add data and server message to struct.
            server_result->success = 0;
            server_result->message = "Failed to write message.";
        }
    }
}

/**
 * Handles the write command in the event handler function.
 * 
 * @param cwd - the current working directory.
 * @param directory_path - the path the the file, excluding file name.
 * @param file_name - the name of the file.
 * @param received_binary_data - the binary data sent from client.
 * @param binary_bytes_received - the size of the binary data sent.
 * @param server result - a struct used to hold data and a server message.
*/
void write_command_handler(char* cwd, char* directory_path, char* file_name, 
                            unsigned char* received_binary_data, int binary_bytes_received, server_result_t* server_result) {
    // Create directories if needed.
    create_directories(cwd, directory_path);

    // Write file to server.
    write_file(cwd, directory_path, file_name, received_binary_data, binary_bytes_received, server_result);
}

/**
 * Reads the file_version file and gets the last line of the file. This is the most recent version.
 * 
 * @param version_file - the file_version file.
 * 
 * @return data about the recent file version in format "file | time".
*/
char* get_most_recent_file_info(char* version_file) {
    pthread_mutex_lock(&file_mutex);
    printf("Version File Path in recent file function: %s\n", version_file);
    FILE* file = fopen(version_file, "r");

    if (file == NULL) {
        pthread_mutex_unlock(&file_mutex);
        return NULL; // Return NULL if file opening fails
    }

    char* recent_file_info = NULL;
    char buffer[1024];

    // Read lines until the last one
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // Check if this line is not empty
        if (buffer[0] != '\0' && buffer[0] != '\n') {
            // Update the recent_file_info each time a non-empty line is found
            if (recent_file_info != NULL) {
                free(recent_file_info);
            }
            recent_file_info = strdup(buffer);
        }
    }

    fclose(file);
    pthread_mutex_unlock(&file_mutex);
    return recent_file_info;
}

/**
 * Gets the size of a file for sending binary data back to client on a GET command.
 * 
 * @param file_path - the path to the file.
 * 
 * @return the size of the file.
*/
size_t get_binary_file_size(char* file_path) {
    pthread_mutex_lock(&file_mutex);
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        exit(EXIT_FAILURE);
    }

    // Read through the file to get it's size.
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    fclose(file);
    pthread_mutex_unlock(&file_mutex);
    return file_size;
}

/**
 * Handles the GET command by checking if the file exists, if it does, it reads the file contents, the file size and adds both
 * to the server_result struct along with the server message.
 * 
 * @param cwd - the current working directory.
 * @param directory_path - the directory path sent by the client.
 * @param file_name - the exact name of the file to get.
 * @param server_result - a struct used to hold the data and server message from any of the server commands.
 * 
 * @return a server_result struct storing data and a server message.
*/
server_result_t* get_command_handler(char* cwd, char* directory_path, char* file_name, server_result_t* server_result) {
    // Create the path to directory that contains the file.
    char* token = strtok(directory_path, "/");
    char path[1024];
    strcpy(path, cwd);

    while (token != NULL) {
        strcat(path, "/");
        strcat(path, token);
        token = strtok(NULL, "/");
    }

    // Generate the path to the file_version file.
    char version_file_name[1024];
    strcat(version_file_name, path);
    strcat(version_file_name, "/");
    strcat(version_file_name, file_name);
    strcat(version_file_name, "_version_history.txt");
    printf("Version path name: %s\n", version_file_name);

    // Check if the file_version file exists. If it doesn't exist then the file doesn't either.
    if (access(version_file_name, F_OK) == -1) {
        server_result->message = "File not found.";
        return server_result;
    } else { // Else, read the file_version file to get the most recent file version and generate the path to that file.
        char* recent_file_info = get_most_recent_file_info(version_file_name);
        char* token = strtok(recent_file_info, " ");
        if (token != NULL) {
            strcat(path, "/");
            strcat(path, token);
        }
    }

    // Open the most recent file version and read the bytes.
    FILE* file = fopen(path, "rb");
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        unsigned char* return_file_contents = (unsigned char*)malloc(file_size * sizeof(unsigned char));
        if (return_file_contents != NULL) {
            fread(return_file_contents, sizeof(unsigned char), file_size, file);
            fclose(file);

            // Add binary file contents and size to server result struct. Add server message to server result struct.
            server_result->data = return_file_contents;
            server_result->size = get_binary_file_size(path);
            server_result->message = "File data successfully retrieved.";
            return server_result;
        } else { // Else, there was an issue reading the file contents.
            fclose(file);
            pthread_mutex_unlock(&file_mutex);
            free(return_file_contents);
            server_result->message = "Error reading file.";
            return server_result;
        }
    } else { // Else, there was an issue opening the file.
        fclose(file);
        pthread_mutex_unlock(&file_mutex);
        server_result->message = "Error opening file.";
        return server_result;
    }
    return NULL;
}


int remove_command_handler(char* cwd, char* directory_path, char* file_name, server_result_t* server_result) {
    // Create the path to directory that contains the file.
    char* token = strtok(directory_path, "/");
    char path[1024];
    strcpy(path, cwd);

    while (token != NULL) {
        strcat(path, "/");
        strcat(path, token);
        token = strtok(NULL, "/");
    }

    // Generate the path to the file_version file.
    char version_file_path[1024];
    strcat(version_file_path, path);
    strcat(version_file_path, "/");
    strcat(version_file_path, file_name);
    strcat(version_file_path, "_version_history.txt");

    // Generate the name of the version_file file.
    char version_file_name[1024];
    strcat(version_file_name, file_name);
    strcat(version_file_name, "_version_history.txt");


    // Check if the file_version file exists. If it doesn't exist then the file doesn't either.
    if (access(version_file_path, F_OK) == -1) {
        server_result->message = "File not found.";
        return -1;
    } else { // Else, generate path to the most recent file version.
        char* recent_file_info = get_most_recent_file_info(version_file_path);
        printf("Recent file info: %s\n", recent_file_info);
        char* token = strtok(recent_file_info, " ");
        if (token != NULL) {
            strcat(path, "/");
            strcat(path, token);
        }
    }

    printf("File Path to remove: %s\n", path);

    // Check if the file version exists and remove it from the server.
    if(access(version_file_path, F_OK) != -1) {
        remove(path);
        server_result->success = 1;
        server_result->message = "File successfully removed from server.";
    } else {
        server_result->message = "Issue removing file from server.";
    }

    // Remove the file version info from the file_version file.
    FILE* file = fopen(version_file_path, "r+");
    if (file != NULL) {
        // Find the first new-line character starting from the end of the file and moving backwards.
        fseek(file, 0, SEEK_END);
        long file_position = ftell(file);
        while (file_position > 0) {
            file_position--;
            fseek(file, file_position, SEEK_SET);
            if (fgetc(file) == '\n') {
                ftruncate(fileno(file), file_position);

            // See if the version_file is empty, if it is delete it.
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            if (file_size <= 1) {
                fclose(file);
                remove(version_file_path);
                return 0;

            } else {
                return -1;
            }
            }
        }
    }
    return -1;
}

void file_version_handler(char* cwd, char* directory_path, char* file_name, server_result_t* server_result) {
    // Create the path to directory that contains the file.
    char* token = strtok(directory_path, "/");
    char path[1024];
    strcpy(path, cwd);

    while (token != NULL) {
        strcat(path, "/");
        strcat(path, token);
        token = strtok(NULL, "/");
    }

    // Generate the path to the file_version file.
    char version_file_path[1024];
    strcat(version_file_path, path);
    strcat(version_file_path, "/");
    strcat(version_file_path, file_name);
    strcat(version_file_path, "_version_history.txt");
    printf("LS file path: %s\n", version_file_path);

    // Check if the file version exists and remove it from the server.
    if(access(version_file_path, F_OK) != -1) {
        printf("Entered because version file exists.");
        FILE* file = fopen(version_file_path, "r");
        if (file == NULL) {
            return;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        char* content = (char*)malloc(file_size + 1);
        if (content == NULL) {
            return;
        }

        size_t read_size = fread(content, sizeof(char), file_size, file);

        content[file_size] = '\0';
        printf("Contents retrieved from file: %s\n", content);

        fclose(file);
        server_result->file_versions = content;
        server_result->success = 1;
        server_result->message = "Version history successfully retrieved.";

    } else {
        server_result->message = "File doesn't exist";
    }
}

/**
 * Handles the various commands sent by the client.
 * 
 * @param action - the command sent by the client, i.e., WRITE, GET, RM, LS.
 * @param cwd - the current working directory.
 * @param directory_path - the directory path sent by the client.
 * @param file_name - the exact name of the file sent by the client.
 * @param binary message - the binary message sent by the client if they want to write.
 * @param binary_bytes_received - the size of the binary message sent by the client if they want to write.
 * 
 * @return a server_result struct storing data and a server message.
*/
server_result_t* event_handler(char* action, char* cwd, char* directory_path, char* file_name, 
                    unsigned char* binary_message, int binary_bytes_received) {

    // Initialize a server_result struct to store data and a server message.
    server_result_t* server_result = (server_result_t*)malloc(sizeof(server_result_t));

    // Check what the command sent by the client was and call various command handlers.
    if (strcmp(action, "WRITE") == 0) {
        write_command_handler(cwd, directory_path, file_name, binary_message, binary_bytes_received, server_result);
        return server_result;
    } else if (strcmp(action, "GET") == 0) {
        get_command_handler(cwd, directory_path, file_name, server_result);
    } else if (strcmp(action, "RM") == 0) {
        remove_command_handler(cwd, directory_path, file_name, server_result);
    } else if (strcmp(action, "LS") == 0) {
        file_version_handler(cwd, directory_path, file_name, server_result);
    }
    return server_result;
}

/**
 * The main part of the program that runs when a thread is spawned by a client connection.
 * 
 * @param socket_desc - the client socket.
*/
void* client_handler(void* socket_desc) {
    int client_socket = *((int*)socket_desc);

    // Receive client message.
    char* client_message_received = (char*)malloc(MESSAGE_SIZE * sizeof(char));
    int bytes_received = recv(client_socket, client_message_received, MESSAGE_SIZE - 1, 0);
    client_message_received[bytes_received] = '\0';
    printf("RECEIVED CLIENT MESSAGE: %s\n", client_message_received);

    // Get Client Action.
    char* action = get_action(client_message_received);

    // If WRITE action, receive binary data.
    unsigned char* binary_message = NULL;
    int binary_bytes_received = 0;
    if (strcmp(action, "WRITE") == 0) {
      binary_message = (unsigned char*)malloc(MAX_BINARY_SIZE * sizeof(unsigned char));
      binary_bytes_received = recv(client_socket, binary_message, MAX_BINARY_SIZE, 0);
    }

    // Get provided remote file path.
    char* remote_file_path = get_remote_file_path(client_message_received);
    // Get directory path excluding file name.
    char* directory_path = get_directory_path(remote_file_path);
    // Get file name.
    char* file_name = get_file_name(remote_file_path);

    // Create path to root folder.
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    char root[5] = "/root";
    strcat(cwd, root);

    // Create root folder if needed.
    create_root_directory(cwd);
    // Create directories if needed.
    create_directories(cwd, directory_path);

    // Handle Response to Client and return a struct holding data and a server message.
    server_result_t* server_result = event_handler(action, cwd, directory_path, file_name, binary_message, binary_bytes_received);

    // Send the server message to the client.
    int send_result = send(client_socket, server_result->message, strlen(server_result->message), 0);

    if (send_result < 0) {
        printf("Failed to send server message\n");
    } else {
        printf("Sent %d bytes to the client\n", send_result);
    }

    // Wait before sending another message.
    sleep(1);

    // If action is GET, send binary data retrieved from file.
    size_t return_binary_file_size;
    if (strcmp(action, "GET") == 0) {
      if (send(client_socket, server_result->data, server_result->size, 0) < 0){
        printf("Can't send\n");
      }
    } else if (strcmp(action, "LS") == 0) {
        if (send(client_socket, server_result->file_versions, strlen(server_result->file_versions), 0) < 0){
            printf("Can't send\n");
      }
    }

    printf("Test Server Message: %s\n", server_result->message);
    printf("LS contents: %s\n", server_result->file_versions);

    // Close client socket and exit thread.
    close(client_socket);
    pthread_exit(NULL);
}