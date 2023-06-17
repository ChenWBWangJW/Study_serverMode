#include <stdio.h>
#include <winsock.h>
#include <errno.h>
#pragma comment(lib, "ws2_32.lib")		// Link with ws2_32.lib

#define BUF_LEN 200
#define PORT 8888

typedef struct sockaddr_in SAIN;
typedef struct sockaddr SA;

void perrorText(const char* str)
{
	fprintf(stderr, "%s error. reason: %d.\n", str, WSAGetLastError());
	exit(-1);
}

int main()
{
	char buf[BUF_LEN];

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)		// Initialize Winsock
		perrorText("WSAStartup");

	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
		perrorText("socket");

	SAIN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("192.168.222.3");

	printf("Connecting to server...\n");
	if (connect(clientSocket, (SA*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		perrorText("connect");
	printf("Connected.\n");

	printf("Input message: ");
	gets_s(buf, BUF_LEN);
	int len = strlen(buf);
	buf[len] = '\0';

	printf("Sending message...\n");
	if (send(clientSocket, buf, len, 0) == SOCKET_ERROR)
		perrorText("send");
	memset(buf, 0, BUF_LEN);		// Clear buffer For Recv(optional

	printf("Receiving message...\n");
	if (recv(clientSocket, buf, BUF_LEN, 0) == SOCKET_ERROR)
		perrorText("recv");
	
	printf("Received message: %s\n", buf);

	closesocket(clientSocket);
	WSACleanup();
	return 0;
}