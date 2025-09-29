/*
        demo-udp-03: udp-recv: a simple udp server
        receive udp messages

        usage:  udp-recv

        Paul Krzyzanowski
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define BUFSIZE 2048
#define SERVICE_PORT 1234

int main(int argc, char **argv)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    struct sockaddr_in myaddr;    /* our address */
    struct sockaddr_in remaddr;   /* remote address */
    int addrlen = sizeof(remaddr);    /* length of addresses */
    int recvlen;                  /* # bytes received */
    char buf[BUFSIZE];            /* buffer for incoming data */
    SOCKET fd;                    /* our socket */
    int msgcnt = 0;

    /* create a UDP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        fprintf(stderr, "cannot create socket\n");
        WSACleanup();
        return 1;
    }

    /* bind the socket to any IP address and our chosen port */
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(SERVICE_PORT);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed\n");
        closesocket(fd);
        WSACleanup();
        return 1;
    }

    printf("UDP server listening on port %d\n", SERVICE_PORT);

    /* loop to receive messages and send acknowledgments */
    while (1) {
        recvlen = recvfrom(fd, buf, BUFSIZE - 1, 0, (struct sockaddr *)&remaddr, &addrlen);
        if (recvlen > 0) {
            buf[recvlen] = 0; // null-terminate
            printf("received from %s : %d message: \"%s\" (%d bytes)\n",
                inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port), buf, recvlen);

            sprintf(buf, "ack %d", msgcnt++);
            printf("sending response \"%s\"\n", buf);
            if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) == SOCKET_ERROR)
                fprintf(stderr, "sendto failed\n");
        } else {
            printf("uh oh - something went wrong!\n");
        }
    }

    /* never exits, but cleanup if it does */
    closesocket(fd);
    WSACleanup();
    return 0;
}
