#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define SERV_PORT 1255
#define MAXLINE 255

int main()
{
    int sockfd, n;
    int len;
    char mesg[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(SERV_PORT); 

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) >= 0) {
        printf("Server is running at port %d\n", SERV_PORT);
    } else {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    for (;;) { 
        len = sizeof(cliaddr); 
        printf("Receiving data ...\n");
        n = recvfrom(sockfd, mesg, MAXLINE, 0,
                     (struct sockaddr *)&cliaddr, &len); 

        if (n == SOCKET_ERROR) {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            break;
        }

        mesg[n] = '\0';
        printf("Received from %s:%d: %s\n",
               inet_ntoa(cliaddr.sin_addr),
               ntohs(cliaddr.sin_port),
               mesg);

        printf("Sending data: %s\n", mesg);
        sendto(sockfd, mesg, n, 0,
               (struct sockaddr *)&cliaddr, len); 
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
