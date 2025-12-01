#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include "../protocol.h"
#include "data_manager.h"
#include "../serialize_helper.h"

// Xử lý client
void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    
    Message msg;
    int bytes_read;
    
    // Thiết lập timeout nhận dữ liệu để tránh bị treo
    struct timeval tv;
    tv.tv_sec = 30; // 30 giây timeout
    tv.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    while ((bytes_read = recv(client_sock, &msg, sizeof(Message), 0)) > 0) {
        Message response;
        memset(&response, 0, sizeof(Message));
        
        switch (msg.command) {
            case CMD_REGISTER:
                if (add_user(msg.email, msg.username, msg.password)) {
                    response.command = RESP_SUCCESS;
                    printf("[REGISTER] Success: %s (%s)\n", msg.username, msg.email);
                } else {
                    response.command = RESP_USER_EXISTS;
                    printf("[REGISTER] Failed: %s (email exists)\n", msg.email);
                }
                send(client_sock, &response, sizeof(Message), 0);
                break;
                
            case CMD_LOGIN:
                if (authenticate(msg.email, msg.password)) {
                    response.command = RESP_SUCCESS;
                    char* token = create_session(msg.email);
                    strcpy(response.access_token, token);
                    free(token);
                    
                    // Lấy username từ email
                    if (!get_username_by_email(msg.email, response.username)) {
                        strcpy(response.username, "Unknown");
                    }
                    
                    printf("[LOGIN] Success: %s (%s)\n", response.username, msg.email);
                } else {
                    response.command = RESP_INVALID_CRED;
                    printf("[LOGIN] Failed: %s\n", msg.email);
                }
                send(client_sock, &response, sizeof(Message), 0);
                break;
                
            case CMD_SEARCH: {
                // Verify token
                if (!verify_token(msg.access_token, msg.email)) {
                    response.command = RESP_INVALID_TOKEN;
                    send(client_sock, &response, sizeof(Message), 0);
                    printf("[SEARCH] Failed: Invalid token for %s\n", msg.email);
                    break;
                }
                
                SearchResponse search_resp = search_files(msg.filename);
                // Gửi struct trực tiếp (sizeof(SearchResponse) bytes)
                send(client_sock, &search_resp, sizeof(SearchResponse), 0);
                printf("[SEARCH] Keyword '%s': %d file(s) found\n", 
                       msg.filename, search_resp.count);
                break;
            }
            
            case CMD_FIND: {
                // Verify token
                if (!verify_token(msg.access_token, msg.email)) {
                    response.command = RESP_INVALID_TOKEN;
                    send(client_sock, &response, sizeof(Message), 0);
                    printf("[FIND] Failed: Invalid token for %s\n", msg.email);
                    break;
                }
                
                FindResponse find_resp = find_peers(msg.filehash);
                
                printf("DEBUG SERVER: Before serialize - Code: %d, Count: %d\n", 
                    find_resp.response_code, find_resp.count);
                if (find_resp.count > 0) {
                    printf("DEBUG SERVER: First peer - IP: %s, Port: %d\n",
                        find_resp.peers[0].ip, find_resp.peers[0].port);
                }
                
                // Serialize trước khi gửi
                char buffer[FIND_RESPONSE_BUFFER_SIZE];
                memset(buffer, 0, FIND_RESPONSE_BUFFER_SIZE);
                serialize_find_response(&find_resp, buffer);
                
                // Dump first 32 bytes
                printf("DEBUG SERVER: First 32 bytes to send: ");
                for (int i = 0; i < 32; i++) {
                    printf("%02x ", (unsigned char)buffer[i]);
                }
                printf("\n");
                
                int sent = send(client_sock, buffer, FIND_RESPONSE_BUFFER_SIZE, 0);
                printf("DEBUG SERVER: Sent %d bytes\n", sent);
                
                printf("[FIND] Hash '%.16s...': %d peer(s) found\n", 
                    msg.filehash, find_resp.count);
                break;
            }
                
            case CMD_PUBLISH:
                // Verify token
                if (!verify_token(msg.access_token, msg.email)) {
                    response.command = RESP_INVALID_TOKEN;
                    send(client_sock, &response, sizeof(Message), 0);
                    printf("[PUBLISH] Failed: Invalid token for %s\n", msg.email);
                    break;
                }
                
                // Validate input
                if (!validate_filename(msg.filename)) {
                    response.command = RESP_INVALID_INPUT;
                    send(client_sock, &response, sizeof(Message), 0);
                    printf("[PUBLISH] Failed: Invalid filename for %s\n", msg.email);
                    break;
                }
                
                publish_file(msg.filename, msg.filehash, msg.email, 
                           msg.ip, msg.port, msg.file_size, msg.chunk_size);
                response.command = RESP_SUCCESS;
                send(client_sock, &response, sizeof(Message), 0);
                printf("[PUBLISH] File: %s (hash: %.16s...) from %s:%d by %s\n", 
                       msg.filename, msg.filehash, msg.ip, msg.port, msg.email);
                break;
                
            case CMD_UNPUBLISH:
                // Verify token
                if (!verify_token(msg.access_token, msg.email)) {
                    response.command = RESP_INVALID_TOKEN;
                    send(client_sock, &response, sizeof(Message), 0);
                    printf("[UNPUBLISH] Failed: Invalid token for %s\n", msg.email);
                    break;
                }
                
                // Check if user owns the file
                if (!is_file_owner(msg.filehash, msg.email)) {
                    response.command = RESP_FILE_NOT_OWNED;
                    send(client_sock, &response, sizeof(Message), 0);
                    printf("[UNPUBLISH] Failed: File %.16s... not owned by %s\n", 
                           msg.filehash, msg.email);
                    break;
                }
                
                unpublish_file(msg.filehash, msg.email);
                response.command = RESP_SUCCESS;
                send(client_sock, &response, sizeof(Message), 0);
                printf("[UNPUBLISH] Hash %.16s... by %s\n", msg.filehash, msg.email);
                break;
                
            case CMD_LOGOUT:
                destroy_session(msg.access_token);
                printf("[LOGOUT] %s\n", msg.email);
                response.command = RESP_SUCCESS;
                send(client_sock, &response, sizeof(Message), 0);
                close(client_sock);
                return NULL;
                
            case CMD_DOWNLOAD_STATUS:
                // Verify token
                if (!verify_token(msg.access_token, msg.email)) {
                    response.command = RESP_INVALID_TOKEN;
                    send(client_sock, &response, sizeof(Message), 0);
                    break;
                }
                
                response.command = RESP_SUCCESS;
                send(client_sock, &response, sizeof(Message), 0);
                
                if (msg.status == 1) {
                    printf("[DOWNLOAD_STATUS] Success: %s downloaded file (hash: %.16s...)\n", 
                           msg.email, msg.filehash);
                } else {
                    printf("[DOWNLOAD_STATUS] Failed: %s failed to download file (hash: %.16s...)\n", 
                           msg.email, msg.filehash);
                }
                break;
        }
    }
    
    close(client_sock);
    return NULL;
}

int main() {
    int server_sock, *client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t thread_id;
    
    printf("=== P2P File Sharing Server ===\n");
    printf("Initializing...\n\n");
    
    // TẢI DỮ LIỆU KHI KHỞI ĐỘNG
    load_data(); 
    
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("Server running on port %d...\n", SERVER_PORT);
    printf("Waiting for connections...\n\n");
    
    while (1) {
        client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        
        if (*client_sock < 0) {
            perror("Accept failed");
            free(client_sock);
            continue;
        }
        
        printf("[CONNECT] New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        pthread_create(&thread_id, NULL, handle_client, client_sock);
        pthread_detach(thread_id);
    }
    
    close(server_sock);
    return 0;
}