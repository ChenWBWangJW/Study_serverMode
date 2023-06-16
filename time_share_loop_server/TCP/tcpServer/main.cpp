/*
* 需求：
* 
* 详解：
* 
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#define BUF_SIZE 200
#define PORT 8888

typedef sockaddr SA;
typedef sockaddr_in SA_IN;

void perrorText(const char* str)
{
	fprintf(stderr, "%s error, reason: %s.\n", str, strerror(errno));
}

int main()
{
	SA_IN fsin;
	int clisock, alen, connum = 0, len, s;
	char buf[BUF_SIZE] = "hi, client!", rbuf[BUF_SIZE];
	struct servent *psp;		/*server information*/
	struct protoent *ppe;		/*protocol information*/
	SA_IN sin;					/*an Internet endpoint address*/
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
	{
		perrorText("socket");
		return -1;
	}

	if (bind(s, (SA*)&sin, sizeof(sin)) == -1)
	{
		perrorText("bind");
		return -1;
	}

	if (listen(s, 5) == -1)
	{
		perrorText("listen");
		return -1;
	}

	while (1)
	{
		alen = sizeof(SA);
		puts("waiting for connection...");
		clisock = accept(s, (SA*)&fsin, (socklen_t*)&alen);
		if (clisock == -1)
		{
			perrorText("accept");
		}

		connum++;
		printf("%d client comes\n", connum);
		len = recv(clisock, rbuf, sizeof(rbuf), 0);
		if (len == -1)
		{
			perrorText("recv");
		}
		else if (len == 0)
		{
			printf("client close the connection\n");
		}
		else
		{
			printf("recv: %s\n", rbuf);
		}
		sprintf(buf, "I have received your message: %s", rbuf);

		send(clisock, buf, sizeof(buf), 0);
		close(clisock);
	}
	return 0;
}