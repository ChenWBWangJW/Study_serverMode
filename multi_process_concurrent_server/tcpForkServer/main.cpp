/*
* 需求：
*	当客户端有请求时，通过创建子进程的方式处理客户端的请求，父进程继续监听端口，等待其他客户端的连接。
* 
* 要点：
*	1.fork();函数的返回值问题，父进程返回子进程的pid，子进程返回0，创建失败返回-1。，因此该函数有两个返回值。
*	每次创建子进程时，父进程和子进程都会执行fork()函数后面的代码，但是父进程和子进程执行的顺序是不确定的。
* 
*	2.且每次创建时，connfd的引用计数器都会+1，但父进程并不具备通信功能，因此父进程在创建子进程后需要关闭一次
*	connfd，使得引用计数器-1，符合实际情况，因为已经将该执行内容交接给子进程处理了，父进程不需要再次处理。
*
*	3.子进程则需要关闭sockfd，因为子进程不需要监听socket，且sockfd是父进程的，因此子进程需要关闭一次sockfd，
*	使得引用计数器-1，
* 
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

typedef struct sockaddr_in SAIN;
typedef struct sockaddr SA;

void perrorText(const char* text)
{
	fprintf(stderr, "%s error, reason: %s\n", text, strerror(errno));
}

int main(int argc, char argv[])
{
	unsigned short port = 8888;
	int on = 1, err_log;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perrorText("socket");
		exit(1);
	}

	//配置本地网络信息
	SAIN saddr;
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	err_log = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (err_log < 0)
	{
		perrorText("setsockopt");
		exit(1);
	}

	//绑定端口
	err_log = bind(sockfd, (SA*)&saddr, sizeof(saddr));
	if (err_log < 0)
	{
		perrorText("bind");
		exit(1);
	}

	//监听端口
	err_log = listen(sockfd, 128);
	if (err_log < 0)
	{
		perrorText("listen");
		exit(1);
	}

	//主进程循环等待客户端连接
	while (1)
	{

		char cli_ip[INET_ADDRSTRLEN] = {0};
		struct sockaddr_in cli_addr;
		socklen_t cliaddr_len = sizeof(cli_addr);
		puts("Father process is waitting client......");

		//等待客户端连接
		int connfd = accept(sockfd, (SA*)&cli_addr, (socklen_t*)&cliaddr_len);
		if (connfd < 0)
		{
			perrorText("accept");
			close(sockfd);
			exit(1);
		}

		pid_t pid = fork();
		if (pid < 0)
		{
			perrorText("fork");
			_exit(1);
		}

		//子进程接收客户端信息，并返回给客户端
		else if (pid == 0)
		{
			close(sockfd);
			char recv_buf[1024] = {0};
			int recv_len = 0;
			char buf[200] = "hi,cilent,I am server!";

			//打印客户端的 ip 和端口
			memset(cli_ip, 0, sizeof(cli_ip));
			inet_ntop(AF_INET, &cli_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);
			printf("----------------------------------------------\n");
			printf("client ip : %s. port: %d\n", cli_ip, ntohs(cli_addr.sin_port));

			//循环接收发送数据
			while ((recv(connfd, recv_buf, sizeof(recv_buf), 0) > 0))
			{
				printf("recv_buf: %s\n", recv_buf);

				//发送数据
				sprintf(buf, "I am server, I have received your message!");
				if ((send(connfd, buf, sizeof(buf), 0) < 0))
				{
					perrorText("send");
					exit(1);
				}
				memset(recv_buf, 0, sizeof(recv_buf));
			}
			
			printf("client_port: %d closed!\n", ntohs(cli_addr.sin_port));
			close(connfd);
			exit(1);
		}

		else if (pid > 0)
		{
			close(connfd);
		}
	}
	close(sockfd);
}