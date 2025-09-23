#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define MAXLINE 4096
#define SERV_PORT 3000

int main(int argc, char **argv) {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];
    int n;

    if (argc != 2) {
        printf("Usage: %s <IP address of the server>\n", argv[0]);
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

    // Chat loop
    while (fgets(sendline, MAXLINE, stdin) != NULL) {
        send(sockfd, sendline, (int)strlen(sendline), 0);

        n = recv(sockfd, recvline, MAXLINE, 0);
        if (n == 0) {
            printf("The server terminated prematurely\n");
            closesocket(sockfd);
            WSACleanup();
            exit(4);
        }
        recvline[n] = '\0'; // null-terminate
        printf("String received from the server: %s", recvline);
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
