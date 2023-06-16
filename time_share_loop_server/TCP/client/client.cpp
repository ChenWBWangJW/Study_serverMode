/*
* 需求：
*
* 详解：
*
*/

#include <stdio.h>
#include <winsock.h>
#pragma comment(lib, "wsock32")

#define BUF_SIZE 200
#define PORT 8888

typedef struct sockaddr SA;
typedef struct sockaddr_in SAIN;

char wbuf[50], rbuf[100];

int main()
{
	char buf[BUF_SIZE];
	SOCKET s;
	int len;
	WSADATA wsadata;

	struct hostent* phe;		/*host information*/
	struct servent* pse;		/*service information*/
	struct protoent* ppe;		/*protocol information*/
	SAIN saddr;					/*server address*/
	int type;					/*socket type*/

	if (WSAStartup(MAKEWORD(2, 0), &wsadata) != 0)
	{
		printf("WSAStartup failed!\n");
		WSACleanup();
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	saddr.sin_addr.s_addr = inet_addr("192.168.222.3");

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("socket failed!\n");
		WSACleanup();
		return -1;
	}

	if (connect(s, (SA*)&saddr, sizeof(saddr)) == SOCKET_ERROR)
	{
		printf("connect failed!\n");
		WSACleanup();
		return -1;
	}

	printf("please enter data:");
	scanf_s("%s", wbuf, sizeof(wbuf));
	len = send(s, wbuf, sizeof(wbuf), 0);
	if (len == SOCKET_ERROR)
	{
		printf("send failed!\n");
		WSACleanup();
		return -1;
	}

	len = recv(s, rbuf, sizeof(rbuf), 0);
	if (len < 0)
	{
		printf("recv failed!\n");
		WSACleanup();
		return -1;
	}
	else if(len == 0)
	{
		printf("connetcion closed!\n");
	}
	else
	{
		printf("%s\n", rbuf);
	}
	closesocket(s);
	WSACleanup();
	return 0;
}