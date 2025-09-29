/*
        demo-udp-03: udp-send: a simple udp client
	send udp messages
	This sends a sequence of messages (the # of messages is defined in MSGS)
	The messages are sent to a port defined in SERVICE_PORT 

        usage:  udp-send

        Paul Krzyzanowski
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define BUFLEN 2048
#define SERVICE_PORT	1234
#define MSGS 5	/* number of messages to send */

int main(void)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed\n");
		return 1;
	}
	struct sockaddr_in myaddr, remaddr;
	SOCKET fd;
	int i, slen=sizeof(remaddr);
	int recvlen;
	char buf[BUFLEN];
	char *server = "127.0.0.1";	/* change this to use a different server */

	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==INVALID_SOCKET) {
		fprintf(stderr, "socket creation failed\n");
		WSACleanup();
		return 1;
	}
	printf("socket created\n");

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0); 

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == SOCKET_ERROR) {
		fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
		closesocket(fd);
		WSACleanup();
		return 1;
	}

	if (inet_pton(AF_INET, server, &remaddr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton() failed\n");
		closesocket(fd);
		WSACleanup();
		return 1;
	}

	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(SERVICE_PORT);

	for (i = 0; i < MSGS; i++) {
		printf("Sending packet %d to %s port %d\n", i, server, SERVICE_PORT);
		sprintf(buf, "This is packet %d", i);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==SOCKET_ERROR) {
			fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
			closesocket(fd);
			WSACleanup();
			return 1;
		}
		/* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
		if (recvlen != SOCKET_ERROR && recvlen >= 0) {
			buf[recvlen] = 0;	/* expect a printable string - terminate it */
			printf("received message: \"%s\"\n", buf);
		}
	}
	closesocket(fd);
	WSACleanup();
	return 0;
}
