/*
* 需求：
*	一个基于windows的客户端程序，能够连接到服务器端，发送消息，接收消息
*/

#include <winsock.h>
#include <stdio.h>
#include <errno.h>
#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 200
#define PORT 8888

char wbuf[50], rbuf[100];

void perrorText(const char* str)
{
	fprintf(stderr, "%s: %d\n", str, WSAGetLastError());
	WSACleanup();
	exit(-1);
}

int main()
{
	char buff[BUF_SIZE];
	SOCKET s;
	int len;
	WSADATA wsadata;

	struct hostent *host;				/*host information*/
	struct servent *serv;				/*service information*/
	struct prototent *protocol;			/*protocol information*/
	struct sockaddr_in server_addr;		/*server address*/

	int type;

	if (WSAStartup(MAKEWORD(2, 0), &wsadata))
	{
		perrorText("WSAStrtup");
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr("192.168.222.3");

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		perrorText("socket");
	}

	if (connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		perrorText("connect");
	}

	printf("please input the message:");
	scanf_s("%s", wbuf, sizeof(wbuf));
	len = send(s, wbuf, sizeof(wbuf), 0);
	if (len < 0)
	{
		perrorText("send");
	}

	len = recv(s, rbuf, sizeof(rbuf), 0);
	if (len < 0)
	{
		perrorText("recv");
	}

	printf("server reply: %s\n", rbuf);
	closesocket(s);
	WSACleanup();
	return 0;
}