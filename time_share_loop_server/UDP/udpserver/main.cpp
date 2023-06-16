/*
*要求：
*	UDP循环服务器每次从套接字上读取一个客户端的请求并将其回送给客户端，直到客户端发送一个空行为止。
*详解：
*	socket(...);
*	bind(...);
* 	while(1)
* 	{
* 		recvfrom(...);
*		process(...);
* 		sendto(...);
*	}
*
*	在该代码中，创建了一个UDP套接字，设置端口复用，绑定端口，然后进入循环，循环中调用recvfrom()函数接收客户端的请求，
*	若没有数据，阻塞等待，若有数据，则调用process()函数处理数据，然后调用sendto()函数将数据回送给客户端。
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

char rbuf[50];
char sbuf[50];

void perrorText(const char* str)
{
	fprintf(stderr, "%s error, reason: %s.\n", str, strerror(errno));
}

int main()
{
	int sockfd, size;
	int on = 1;
	SA_IN saddr;
	SA_IN raddr;

	//设置地址信息，ip信息
	size = sizeof(SA_IN);
	memset(&saddr, 0, size);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8888);
	saddr.sin_addr.s_addr = inet_addr("192.168.222.3");

	//创建UDP套接字
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perrorText("socket");
		return -1;
	}

	//设置套接字选项:地址复用
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) == -1)
	{
		perrorText("setsockopt");
		return -1;
	}

	//绑定套接字
	if ((bind(sockfd, (SA*)&saddr, sizeof(SA))) < 0)
	{
		perrorText("bind");
		return -1;
	}
	int val = sizeof(SA);
	
	//循环接收客户端信息
	while (1)
	{
		puts("----------wait for client's request----------");

		//接收客户端信息
		if ((recvfrom(sockfd, rbuf, 50, 0, (SA*)&raddr, (socklen_t*)&val)) < 0)
		{
			perrorText("recvfrom");
			return -1;
		}
		printf("Recv from %s:%d.\ndata: %s\n", inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), rbuf);
		sprintf(sbuf, "server has received your data: %s", rbuf);

		//发送信息给客户端
		if((sendto(sockfd, sbuf, strlen(sbuf), 0, (SA*)&raddr, val)) < 0)
		{
			perrorText("sendto");
			return -1;
		}
		memset(rbuf, 0, sizeof(rbuf));
		memset(sbuf, 0, sizeof(sbuf));
	}
	close(sockfd);
	getchar();
	return 0;
}