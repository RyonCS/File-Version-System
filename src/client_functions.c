#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* get_action(char* argv[]) {
    // Check if the action argument is a valid action.
    if ((strcmp(argv[1], "WRITE") != 0) && (strcmp(argv[1], "GET") != 0) &&
        (strcmp(argv[1], "RM") != 0) && (strcmp(argv[1], "LS") != 0)) {
        printf("Not a valid ACTION command.");
        exit(EXIT_FAILURE);
    } else {
        return argv[1];
    }
}

char* get_remote_file_name(int argc, char* argv[], char* action) {
    if (strcmp(action, "WRITE") == 0) {
        if (argc == 3) {
            return argv[2];
        } else {
            return argv[3];
        }
    } else if (strcmp(action, "GET") == 0 || strcmp(action, "RM") == 0 || strcmp(action, "LS") == 0) {
        return argv[2];
    } else {
        return NULL;
    }
}

char* get_local_file_name(int argc, char* argv[], char* action) {
    if (strcmp(action, "WRITE") == 0) {
        return argv[2];
    } else if (strcmp(action, "GET") == 0) {
        if (argc == 4) {
            return argv[3];
        } else {
            char* current_directory = (char*)malloc(1024 * sizeof(char));
            if (current_directory != NULL) {
                getcwd(current_directory, strlen(current_directory));
                return current_directory;
            } else {
                exit(EXIT_FAILURE);
            }
        }
    } else {
        return NULL;
    }
}

unsigned char* read_local_file(char* file_name) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    unsigned char* file_contents = (unsigned char*)malloc(file_size * sizeof(unsigned char));
    if (file_contents == NULL) {
        fclose(file);
        return NULL;
    }

    fread(file_contents, sizeof(unsigned char), file_size, file);

    fclose(file);
    return file_contents;
}

size_t get_binary_file_size(char* file_name) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    fclose(file);
    return file_size;
}

char* generate_client_message(int argc, char* argv[], char* action) {
    char* remote_file_name = get_remote_file_name(argc, argv, action);
    int message_length = strlen(action) + strlen(remote_file_name) + 2;

    char* client_message = (char*)malloc(message_length * sizeof(char));

    strcpy(client_message, action);
    strcat(client_message, "|");
    strcat(client_message, remote_file_name);
    return client_message;
    return NULL;
}

void get_command_handler(char* local_file_name, char* remote_file_name, int argc, 
                        unsigned char* binary_message_received, int bytes_received) {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    char path[1024];
    strcpy(path, cwd);

    if (argc == 4) {
        strcat(path, "/");
        strcat(path, local_file_name);
    } else {
        strcat(path, "/");
        char* file_name = strrchr(remote_file_name, '/');
        if (file_name != NULL) {
            file_name++;
            strcat(path, file_name);
        } else {
            strcat(path, remote_file_name);
        } 
    }
    printf("File path after GET response: %s\n", path);

    FILE* file = fopen(path, "wb");
    if (file != NULL) {
        fwrite(binary_message_received, sizeof(unsigned char), bytes_received, file);
        fclose(file);
    }
    fclose(file);
    return;
}
