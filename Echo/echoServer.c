#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define MAXLINE 4096
#define SERV_PORT 3000
#define LISTENQ 8

int main() {
    WSADATA wsaData;
    SOCKET listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    int n;
    int clilen;
    char buf[MAXLINE];
    char filename[MAXLINE];
    FILE *fp;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    // Setup server address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    // Bind
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(listenfd);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(listenfd, LISTENQ) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(listenfd);
        WSACleanup();
        return 1;
    }

    printf("Server running... waiting for connections on port %d\n", SERV_PORT);

    while (1) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        if (connfd == INVALID_SOCKET) {
            printf("Accept failed\n");
            continue;
        }
        printf("Received connection from: %s port %d\n", 
               inet_ntoa(cliaddr.sin_addr), 
               ntohs(cliaddr.sin_port));

        int fname_len = recv(connfd, filename, MAXLINE, 0);
        if (fname_len <= 0) {
            printf("Failed to receive file name\n");
            closesocket(connfd);
            continue;
        }
        filename[fname_len] = '\0';

        // Prepare saved file name
        char saved_filename[MAXLINE * 2];
        snprintf(saved_filename, sizeof(saved_filename), "saved_file:%s", filename);

        fp = fopen(saved_filename, "wb");
        if (fp == NULL) {
            printf("Cannot open file to write\n");
            closesocket(connfd);
            continue;
        }

        // Receive file data and write to file
        while ((n = recv(connfd, buf, MAXLINE, 0)) > 0) {
            fwrite(buf, 1, n, fp);
        }

        printf("File received and saved as %s\n", saved_filename);
        fclose(fp);
        closesocket(connfd);
    }

    closesocket(listenfd);
    WSACleanup();
    return 0;
}
