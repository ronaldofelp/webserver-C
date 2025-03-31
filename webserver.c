#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 3030
#define BUFFER_LEN 2048
#define WEBROOT "."

void send_error_response(SOCKET client_fd, const char* status_code, const char* message) {
    char error_response[BUFFER_LEN];
    snprintf(error_response, sizeof(error_response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s", status_code, (int)strlen(message), message);
    send(client_fd, error_response, strlen(error_response), 0);
    fprintf(stderr, "Sent error response: %s\n", status_code);
}

const char* get_content_type(const char* filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "application/octet-stream";
    }
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
        return "text/html";
    }
    if (strcmp(dot, ".css") == 0) {
        return "text/css";
    }
    if (strcmp(dot, ".js") == 0) {
        return "application/javascript";
    }
    return "application/octet-stream";
}

int main() {

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Erro ao iniciar Winsock: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char request_buffer[BUFFER_LEN] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) { 

        fprintf(stderr, "Erro socket: %d\n", WSAGetLastError()); 
        WSACleanup(); 
        return 1; }

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) { 

        fprintf(stderr, "Erro setsockopt: %d\n", WSAGetLastError()); 

        closesocket(server_fd); 

        WSACleanup(); 

        return 1; 
    
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) { 

        fprintf(stderr, "Erro bind: %d\n", WSAGetLastError()); 
        closesocket(server_fd); WSACleanup(); 
        return 1; 
    }

    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) { 

        fprintf(stderr, "Erro listen: %d\n", WSAGetLastError()); 
        closesocket(server_fd); WSACleanup(); 
        return 1; 
    }

    printf("WebServer iniciado. Escutando na porta: %d\n", PORT);
    printf("Raiz web definida em: %s\n", WEBROOT);

    bool cont = true;
    while (cont) {
        printf("\nAguardando conexao...\n");
        client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd == INVALID_SOCKET) {
            fprintf(stderr, "Erro ao aceitar conexao: %d\n", WSAGetLastError());
            continue;
        }
        printf("Conexao recebida!\n");

        memset(request_buffer, 0, BUFFER_LEN);
        int byteCount = recv(client_fd, request_buffer, BUFFER_LEN - 1, 0);
        if (byteCount <= 0) {
            if (byteCount == 0) printf("Cliente desconectou.\n");
            else fprintf(stderr, "Erro ao ler dados do cliente: %d\n", WSAGetLastError());
            closesocket(client_fd);
            continue;
        }
        request_buffer[byteCount] = '\0';

        char method[16], uri[BUFFER_LEN], version[16];
        if (sscanf(request_buffer, "%15s %2047s %15s", method, uri, version) < 3) {
            fprintf(stderr, "Formato de requisicao invalido.\n");
            send_error_response(client_fd, "400 Bad Request", "Requisicao mal formatada.");
            closesocket(client_fd);
            continue;
        }

        printf("Recurso solicitado: %s\n", uri);

        char file_path[BUFFER_LEN + 128];
        char *requested_file = uri;

        if (strcmp(requested_file, "/") == 0) {
            requested_file = "/index.html";
        }

        snprintf(file_path, sizeof(file_path), "%s%s", WEBROOT, requested_file);

        printf("Tentando servir o arquivo: %s\n", file_path);

        FILE *file_to_serve = fopen(file_path, "rb");
        if (file_to_serve == NULL) {
            perror("Erro ao abrir arquivo solicitado");
            send_error_response(client_fd, "404 Not Found", "O recurso solicitado nao foi encontrado.");
            closesocket(client_fd);
            continue;
        }

        fseek(file_to_serve, 0, SEEK_END);
        long file_size = ftell(file_to_serve);
        fseek(file_to_serve, 0, SEEK_SET);

        if (file_size < 0) {
            perror("Erro ao obter tamanho do arquivo");
            fclose(file_to_serve);
            send_error_response(client_fd, "500 Internal Server Error", "Erro ao ler arquivo.");
            closesocket(client_fd);
            continue;
        }

        char *file_content_buffer = (char *)malloc(file_size);
        if (file_content_buffer == NULL) {
            perror("Erro ao alocar memoria para o conteudo do arquivo");
            fclose(file_to_serve);
            send_error_response(client_fd, "500 Internal Server Error", "Erro interno (memoria).");
            closesocket(client_fd);
            continue;
        }

        size_t bytes_read = fread(file_content_buffer, 1, file_size, file_to_serve);
        fclose(file_to_serve);

        if (bytes_read != (size_t)file_size) {
            perror("Erro ao ler conteudo do arquivo");
            free(file_content_buffer);
            send_error_response(client_fd, "500 Internal Server Error", "Erro ao ler arquivo.");
            closesocket(client_fd);
            continue;
        }

        const char* content_type = get_content_type(file_path);
        printf("Content-Type: %s\n", content_type);

        char *http_response = NULL;
        size_t header_len = 0;
        size_t response_size = 0;

        const char *header_fmt = "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: %s\r\n"
                                 "Content-Length: %ld\r\n"
                                 "Connection: close\r\n"
                                 "\r\n";

        header_len = snprintf(NULL, 0, header_fmt, content_type, file_size);
        response_size = header_len + file_size;

        http_response = (char *)malloc(response_size);
        if (http_response == NULL) {
            perror("Erro ao alocar memoria para a resposta HTTP");
            free(file_content_buffer);
            send_error_response(client_fd, "500 Internal Server Error", "Erro interno (memoria).");
            closesocket(client_fd);
            continue;
        }

        snprintf(http_response, header_len + 1, header_fmt, content_type, file_size);

        memcpy(http_response + header_len, file_content_buffer, file_size);

        int bytes_sent = send(client_fd, http_response, response_size, 0);
        if (bytes_sent == SOCKET_ERROR) {
            fprintf(stderr, "Erro ao enviar resposta: %d\n", WSAGetLastError());
        } else {
            printf("Resposta enviada (%zu bytes) para %s!\n", response_size, requested_file);
        }

        free(file_content_buffer);
        free(http_response);
        closesocket(client_fd);
        printf("Conexao fechada.\n");

    }

    closesocket(server_fd);
    WSACleanup();
    printf("WebServer parado.\n");
    return 0;
}