#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define SERV_PORT 1255
#define MAXLINE 250

int main()
{
    int sockfd, n;
    struct sockaddr_in servaddr, from_socket;
    int addrlen = sizeof(from_socket);
    char sendline[MAXLINE], recvline[MAXLINE + 1];

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(SERV_PORT); 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Type messages to send to the server:\n");
    
    while (fgets(sendline, MAXLINE, stdin) != NULL) { 
        printf("To Server: %s", sendline);

        sendto(sockfd, sendline, strlen(sendline), 0,
               (struct sockaddr *)&servaddr, sizeof(servaddr));

        n = recvfrom(sockfd, recvline, MAXLINE, 0,
                     (struct sockaddr *)&from_socket, &addrlen);

        if (n == SOCKET_ERROR) {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            break;
        }

        recvline[n] = 0; // null terminate
        printf("From Server: %s:%d %s\n",
               inet_ntoa(from_socket.sin_addr),
               ntohs(from_socket.sin_port),
               recvline);
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
