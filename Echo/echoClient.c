#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define MAXLINE 4096
#define SERV_PORT 3000

int main(int argc, char **argv) {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];
    int n;
    FILE *fp;

    if (argc != 3) {
        printf("Usage: %s <IP address of the server> <file path>\n", argv[0]);
        exit(1);
    }

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Problem in creating the socket\n");
        WSACleanup();
        exit(2);
    }

    // Setup server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(SERV_PORT);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("Problem in connecting to the server\n");
        closesocket(sockfd);
        WSACleanup();
        exit(3);
    }

    // Open file
    fp = fopen(argv[2], "rb");
    if (fp == NULL) {
        printf("Cannot open file: %s\n", argv[2]);
        closesocket(sockfd);
        WSACleanup();
        exit(4);
    }

    // Send file name first 
   /* const char *filepath = argv[2];
    const char *basename = strrchr(filepath, '\\');
    if (!basename) basename = strrchr(filepath, '/');
    basename = basename ? basename + 1 : filepath;
    int fname_len = strlen(basename);
    if (send(sockfd, basename, fname_len, 0) != fname_len) {
        printf("Error sending file name\n");
        fclose(fp);
        closesocket(sockfd);
        WSACleanup();
        exit(5);
    }*/

    // Send file contents
    while ((n = fread(sendline, 1, MAXLINE, fp)) > 0) {
        int sent = 0;
        while (sent < n) {
            int bytes = send(sockfd, sendline + sent, n - sent, 0);
            if (bytes == SOCKET_ERROR) {
                printf("Error sending file data\n");
                fclose(fp);
                closesocket(sockfd);
                WSACleanup();
                exit(6);
            }
            sent += bytes;
        }
    }

    printf("File sent successfully.\n");

    fclose(fp);
    closesocket(sockfd);
    WSACleanup();
    return 0;
}
