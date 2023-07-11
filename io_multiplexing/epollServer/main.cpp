/**********************************
*
***********************************/

#include <ctype.h>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>		//for close

#define MAXLINE 80
#define SERV_PORT 8888
#define OPEN_MAX 1024

typedef struct sockaddr SA;
typedef struct sockaddr_in SAIN;

void perrorText(const char* str)
{
	fprintf(stderr, "%s errno. reason: %s\n", str, strerror(errno));
}

int main(int argc, char* argv[])
{
	int i, j, maxi, listenfd, connfd, sockfd;			//maxi用于存储client[]最大不空闲下标值
	int nready, efd, res;								//nready用于存储epoll_wait返回值，表示返回文件描述符个数； efd指向epoll_create返回的文件描述符； res用于存储epoll_ctl返回值
	ssize_t n;											//ssize_t是有符号的size_t类型,用于存储read返回值
	char buf[MAXLINE], str[INET_ADDRSTRLEN];			//buf用于存储read返回的数据；str用于存储客户端IP地址
	socklen_t clilen;									//clilen用于存储accept返回的客户端地址长度	
	int client[OPEN_MAX];								//用于存储客户端套接字描述符
	SAIN servaddr, cliaddr;
	struct epoll_event tep, ep[OPEN_MAX];				//tep: epoll_ctl参数  ep[] : epoll_wait参数,用于wait函数返回要处理的事件并存入其中

	//1.网络socket初始化
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof(servaddr));				//bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);				//主机字节序转换为网络字节序
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);		//INADDR_ANY表示本地任意IP地址

	if ((bind(listenfd, (SA*)&servaddr, sizeof(servaddr))) == -1)
	{
		perrorText("bind");
		exit(1);
	}

	if ((listen(listenfd, 128)) == -1)
	{
		perrorText("listen");
		exit(1);
	}
	fprintf(stderr, "listen OK\n");

	//2.epoll初始化
	for (i - 0; i < OPEN_MAX; i++)
	{
		client[i] = -1;					//用-1初始化存放客户端套接字数组client[]
	}
	maxi = -1;							//client[]的下标,后面数据初始化赋值时使用，数据初始化为-1
	efd = epoll_create(OPEN_MAX);		//创建epoll模型, efd指向红黑树根节点
	if (efd == -1)
	{
		perrorText("epoll create");
	}

	//3.添加监听描述符事件
	tep.events = EPOLLIN;		//监听读事件
	tep.data.fd = listenfd;
	res = epoll_ctl(efd, EPOLL_CTL_ADD,listenfd, &tep);		//将listenfd加入红黑树, efd:epoll文件描述符, EPOLL_CTL_ADD:注册新的fd到epfd中, listenfd:需要监听的fd, &tep:告诉内核需要监听什么事
	if (res == -1)
	{
		perrorText("epoll ctl");
	}

	//4.循环等待客户端连接请求事件的发生
	for (; ;)
	{
		nready = epoll_wait(efd, ep, OPEN_MAX, -1);		//阻塞监听事件的发生, efd:epoll文件描述符, ep:返回的事件集合, OPEN_MAX:监听事件个数, -1:永久阻塞
		if (nready == -1)
		{
			perrorText("epoll wait");
		}

		//如果有事发生则开始处理
		for (i = 0; i < nready; i++)
		{
			//如果不是读事件则继续
			if (!(ep[i].events & EPOLLIN))
				continue;

			//若处理的时间和文件描述符相等，开始数据处理
			if (ep[i].data.fd == listenfd)		//判断事件是否来自监听套接字
			{
				//接受客户端
				clilen = sizeof(cliaddr);
				connfd = accept(listenfd, (SA*)&cliaddr, &clilen);
				if (connfd == -1)
				{
					perrorText("accept");
					exit(1);
				}

				//将新的客户端添加到client[]数组中
				printf("received from %s at PORT %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));
				for (j = 0; j < OPEN_MAX; j++)
					if (client[j] < 0)
					{
						client[j] = connfd;		//保存客户端描述符
						break;
					}

				//判断是否到达最大值，保护判断
				if (j == OPEN_MAX)
				{
					perrorText("too many clients\n");
					exit(1);
				}

				//更新client下标
				if(j > maxi)
					maxi = j;

				//添加通信套接字到树(底层是红黑树)上
				//内核是否有epoll.evert类型的结构体用于存储传入参数？？？未知待解
				tep.events = EPOLLIN;
				tep.data.fd = connfd;
				res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);		//将connfd加入红黑树, efd:epoll文件描述符, EPOLL_CTL_ADD:注册新的fd到epfd中, connfd:需要监听的fd, &tep:告诉内核需要监听什么事
				if (res == -1)
				{
					perrorText("epoll ctl");
				}
			}
			else
			{
				//不是监听套接字，是通信套接字，开始处理数据
				sockfd = ep[i].data.fd;
				n = read(sockfd, buf, MAXLINE);
				if (n == 0)		//无数据删除该节点
				{
					//将client中对应fd数据值恢复为-1
					for (j = 0; j <= maxi; j++)
					{
						if (client[j] == sockfd)
						{
							client[j] = -1;
							break;
						}
					}

					//将该节点从红黑树上移除
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);		//将该节点从红黑树上移除, efd:epoll文件描述符, EPOLL_CTL_DEL:从epfd中删除一个fd, sockfd:需要监听的fd, NULL:告诉内核不需要监听什么事
					if (res == -1)
					{
						perrorText("epoll ctl");
					}
					close(sockfd);
					printf("client[%d] closed connection\n", j);
				}
				else
				{
					//写回给客户端
					printf("receive client's data: %s\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port), buf);
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);		//简单的字母大小写转换
					write(sockfd, buf, n);				//写回给客户端
				}
			}
		}
	}
	close(listenfd);
	close(efd);
	return 0;
}