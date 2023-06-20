/**********************************
* File: main.cpp
* Brief: 服务器端
* 需求：
*	服务器端使用select()函数来实现多客户端连接
* 
* 实现细节：
* 1.声明一个文件描述符集合fd_set，用于存放所有的文件描述符
* 
* 2.初始化文件描述符集合
* 
* 3.设置超时时间
* 
* 4.添加活动的连接到文件描述符集合中
* 
* 5.调用select(),如果有活动连接,返回活动连接数量，处理活动连接，实现I/O多路复用和多用户的连接通信
* 
* 6.如果没有则阻塞客户端套接字等待并进行轮询查看是否有活动连接
* 
* 7.检查是否有新的连接请求
* 
* 8.如果有则接受连接
* 
* 9.将新的连接添加到文件描述符集合中，判断有效连接数是否已达到最大值，如果没有达到则将新的连接添加到集合中
* 
* 10.如果达到最大值则关闭新的连接
* 
* 11.返回第2步
* 
* 相关函数：
* 1.void FD_ZERO(fd_set* fdset)：清空文件描述符集合
* 
* 2.void FD_SET(int fd, fd_set* fdset)：将文件描述符添加到文件描述符集合中
* 
* 3.void FD_CLR(int fd, fd_set* fdset)：将文件描述符从文件描述符集合中删除
* 
* 4.int FD_ISSET(int fd, fd_set* fdset)：检查文件描述符是否在文件描述符集合中
* 
* 5.select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout)：监视文件描述符集合中的套接字，当集合中有套接字发生变化时，select()函数返回，否则一直阻塞
***********************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr_in SAIN;
typedef struct sockaddr SA;

#define MTPORT 8888		//连接时使用端口
#define MAX_CLIEN_LINE 5		//最大连接数
#define	BUF_SIZE 200

int fd[MAX_CLIEN_LINE];		//文件描述符集合
int connect_amount;

void perrorText(const char* str)
{
	fprintf(stderr, "%s error. reason: %s\n", str, strerror(errno));
	printf("Type any keyboard for skip......\n");
	getchar();
	exit(-1);
}

void showclient()
{
	int i;
	printf("client amount:%d\n", connect_amount);
	for (i = 0; i < MAX_CLIEN_LINE; i++)
	{
		printf("[%d]:%d ", i + 1, fd[i]);
	}
	printf("\n\n");
}

int main(void)
{
	int sockfd, new_fd;		//sockfd为监听socket，new_fd为连接socket
	SAIN server_addr;		//服务器地址
	SAIN client_addr;		//客户端地址
	socklen_t sin_size;		//地址长度
	int yes = 1;			//用于setsockopt()函数选项
	char buf[BUF_SIZE];		//缓冲区
	int ret;				//返回值
	int i;					//循环变量

	//建立socket
	printf("creating socket......\n");
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perrorText("socket");
	}
	printf("create socket success.\n");

	//设置socket属性
	//SOK_SOCKER 指定系统中解释选项的级别为普通套接字
	//SO_REUSEADDR 允许同一端口上启动同一服务器的多个实例
	printf("setting socket......\n");
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1)
	{
		perrorText("setsockopt");
	}
	printf("setsockopt success.\n");

	//配置socket地址
	printf("initializing socket addr......\n");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(MTPORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));
	printf("initiate socket addr success.\n");

	//绑定socket
	printf("Binding socket and server addr......\n");
	if ((bind(sockfd, (SA*)&server_addr, sizeof(SA))) == -1)
	{
		perrorText("bind");
	}
	printf("Bind success.\n");

	//监听socket
	printf("Start listening.\n");
	if ((listen(sockfd, MAX_CLIEN_LINE)) == -1)
	{
		perrorText("listen");
	}
	printf("listen port: %d\nListening......\n", server_addr.sin_port);

	//文件描述符集合
	fd_set fdsr;					//文件描述符集合
	int maxsock;					//最大文件描述符
	struct timeval tv;				//超时时间
	connect_amount = 0;				//连接数
	sin_size = sizeof(SAIN);		//地址长度
	maxsock = sockfd;				//最大文件描述符为监听socket

	while (1)
	{
		//初始化文件描述符集合
		FD_ZERO(&fdsr);
		FD_SET(sockfd, &fdsr);

		//设置超时时间
		tv.tv_sec = 30;
		tv.tv_usec = 0;

		//添加活动的连接到集合中
		for (i = 0; i < MAX_CLIEN_LINE; i++)
		{
			if (fd[i] != 0)
			{
				FD_SET(fd[i], &fdsr);
			}
		}

		//如果文件描述符中有链接请求，则会做出相应的处理，实现I/O复用和多用户的连接通信
		ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
		if (ret < 0)
		{
			perrorText("select");
		}
		else if (ret == 0)		//指定时间到了
		{
			printf("timeout\n");
			continue;
		}

		//轮询各个文件描述符
		for (i = 0; i < connect_amount; i++)
		{
			if (FD_ISSET(fd[i], &fdsr))
			{
				ret = recv(fd[i], buf, sizeof(buf), 0);
				if (ret <= 0)
				{
					printf("client[%d] close\n", i + 1);
					close(fd[i]);
					FD_CLR(fd[i], &fdsr);
					fd[i] = 0;
					connect_amount --;
				}

				//否则有相应的数据发送过来
				else
				{
					if (ret < BUF_SIZE)
					{
						memset(&buf[ret], '\0', 0);
					}

					printf("client[%d] send:%s\n", i + 1, buf);
					send(fd[i], buf, sizeof(buf), 0);
				}
			}
		}

		//检查是否有新的连接请求
		if (FD_ISSET(sockfd, &fdsr))
		{
			new_fd = accept(sockfd, (SA*)&client_addr, &sin_size);
			if (new_fd <= 0)
			{
				perror("accept");
				continue;
			}

			//将新的连接添加到文件描述符集合中，判断有效连接数是否已达到最大值，如果没有达到则将新的连接添加到集合中
			if (connect_amount < MAX_CLIEN_LINE)
			{
				for (i = 0; i < MAX_CLIEN_LINE; i++)
				{
					if (fd[i] == 0)
					{
						fd[i] = new_fd;
						break;
					}
				}
				connect_amount ++;
				printf("New connection client[%d] %s:%d\n", connect_amount, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
				if (new_fd > maxsock)
				{
					maxsock = new_fd;
				}
			}
			else
			{
				printf("max connections arrive, exit\n");
				send(new_fd, "bye", 4, 0);
				close(new_fd);
				continue;
			}
		}
		printf("showing client data: \n");
		showclient();
	}

	for (i = 0; i < MAX_CLIEN_LINE; i++)
	{
		if (fd[i] != 0)
		{
			close(fd[i]);
		}
	}
	return 0;
}