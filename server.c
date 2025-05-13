#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define DEFAULT_PORT 80
#define STATIC_DIR "./static"

void *handle_client(void *client_socket);
void send_response(int client_socket, int status_code, const char *status_message, const char *body, const char *content_type);
void serve_static_file(int client_socket, const char *path);
void handle_calc(int client_socket, const char *path);
void handle_sleep(int client_socket, const char *path);
void handle_request(int client_socket);

int port = DEFAULT_PORT;

int main(int argc, char *argv[]) {
    if (argc > 2 && strcmp(argv[1], "-p") == 0) {
        port = atoi(argv[2]);
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        exit(1);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen");
        exit(1);
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("Failed to accept connection");
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)(intptr_t)client_socket);
    }

    return 0;
}

void *handle_client(void *client_socket) {
    int sock = (intptr_t)client_socket;
    handle_request(sock);
    close(sock);
    return NULL;
}

void handle_request(int client_socket) {
    char request[1024];
    int bytes_received = recv(client_socket, request, sizeof(request) - 1, 0);
    if (bytes_received < 0) {
        perror("Failed to read request");
        return;
    }

    request[bytes_received] = '\0';  // Null-terminate the request string
    printf("Request:\n%s\n", request);

    char method[10], path[256];
    sscanf(request, "%s %s", method, path);

    if (strcmp(method, "GET") == 0) {
        if (strncmp(path, "/static/", 8) == 0) {
            serve_static_file(client_socket, path);
        } else if (strncmp(path, "/calc/", 6) == 0) {
            handle_calc(client_socket, path);
        } else if (strncmp(path, "/sleep/", 7) == 0) {
            handle_sleep(client_socket, path);
        } else {
            send_response(client_socket, 404, "Not Found", "Page not found", "text/html");
        }
    } else {
        send_response(client_socket, 405, "Method Not Allowed", "Only GET method is allowed", "text/html");
    }
}

void serve_static_file(int client_socket, const char *path) {
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", STATIC_DIR, path + 7);  // Remove "/static"
    
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        send_response(client_socket, 404, "Not Found", "File not found", "text/html");
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    char *file_data = malloc(file_stat.st_size);
    read(file_fd, file_data, file_stat.st_size);
    close(file_fd);

    send_response(client_socket, 200, "OK", file_data, "application/octet-stream");
    free(file_data);
}

void handle_calc(int client_socket, const char *path) {
    int num1, num2;
    char op[4];
    int result = 0;
    int valid = sscanf(path, "/calc/%3s/%d/%d", op, &num1, &num2);

    if (valid != 3) {
        send_response(client_socket, 400, "Bad Request", "Invalid calculation format", "text/html");
        return;
    }

    if (strcmp(op, "add") == 0) {
        result = num1 + num2;
    } else if (strcmp(op, "mul") == 0) {
        result = num1 * num2;
    } else if (strcmp(op, "div") == 0) {
        if (num2 == 0) {
            send_response(client_socket, 400, "Bad Request", "Cannot divide by zero", "text/html");
            return;
        }
        result = num1 / num2;
    } else {
        send_response(client_socket, 400, "Bad Request", "Unknown operation", "text/html");
        return;
    }

    char result_str[50];
    snprintf(result_str, sizeof(result_str), "Result: %d", result);
    send_response(client_socket, 200, "OK", result_str, "text/html");
}

void handle_sleep(int client_socket, const char *path) {
    int seconds;
    if (sscanf(path, "/sleep/%d", &seconds) != 1) {
        send_response(client_socket, 400, "Bad Request", "Invalid seconds format", "text/html");
        return;
    }

    sleep(seconds);
    send_response(client_socket, 200, "OK", "Slept successfully", "text/html");
}

void send_response(int client_socket, int status_code, const char *status_message, const char *body, const char *content_type) {
    char response[2048];
    int content_length = body ? strlen(body) : 0;

    snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s", status_code, status_message, content_type, content_length, body);

    send(client_socket, response, strlen(response), 0);
}
