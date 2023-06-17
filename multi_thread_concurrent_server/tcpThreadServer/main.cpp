/*
* 遗留问题：
*	为什么线程client_process()函数中printf()函数无法打印ip地址
*/

#include <cstdio>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

typedef struct sockaddr_in SAIN;
typedef struct sockaddr SA;

#define BUF_LEN 200

typedef struct 
{
	int connfd;
	int port;
	char ip[INET_ADDRSTRLEN] = "";
}ADDRINFORM;

void perrorText(const char* str)
{
	fprintf(stderr, "%s error. reason %s\n", str, strerror(errno));
	exit(-1);
}

/*
* 在使用 void* 型指针传递参数时，需要注意以下几点：
*
* 1.在主函数中，需要将参数的地址传递给 pthread_create 函数，即 pthread_create(&thread_id, NULL, thread_func, &arg)。
*
* 2.在线程函数中，需要将 void* 型指针转换为实际的数据类型，即 int*、char* 或者其他类型。
*
* 3.在线程函数中，需要注意数据类型的大小和内存对齐问题，以避免出现访问越界或者内存泄漏等问题。
*/
void* client_process(void* arg)
{
	int recv_len = 0;
	char recv_buf[BUF_LEN] = "";
	ADDRINFORM *p = (ADDRINFORM*)arg;
	int connfd = (int)p->connfd;		//通过参数传递套接字描述符

	//接收数据
	while (recv_len = recv(connfd, recv_buf, BUF_LEN, 0) > 0)
	{
		printf("recv_buf: %s\n", recv_buf);
		memset(recv_buf, 0, BUF_LEN);
		sprintf(recv_buf, "hi, client, I have received your data!");
		if (send(connfd, recv_buf, BUF_LEN, 0) < 0)
		{
			perrorText("send");
		}
	}

	printf("client ip: %s, port: %d closed!\n", p->ip, p->port);
	close(connfd);
	return NULL;
}

int main()
{
	int sockfd = 0, connfd = 0, err_log = 0;
	int on = 1;
	SAIN my_addr;
	unsigned short port = 8888;
	pthread_t thread_id;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perrorText("socket");
	}

	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	err_log = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (err_log < 0)
	{
		perrorText("setsockopt");
	}

	printf("Binding server to port %d\n", port);
	err_log = bind(sockfd, (SA*)&my_addr, sizeof(my_addr));
	if (err_log < 0)
	{
		perrorText("bind");
	}

	printf("Server listening...\n");
	err_log = listen(sockfd, 128);
	if (err_log < 0)
	{
		perrorText("listen");
	}

	//循环接收客户端连接
	while (1)
	{
		char cli_ip[INET_ADDRSTRLEN] = "";
		SAIN client_addr;
		ADDRINFORM addrinform;
		socklen_t cliaddr_len = sizeof(client_addr);
		connfd = accept(sockfd, (SA*)&client_addr, &cliaddr_len);
		if (connfd < 0)
		{
			perrorText("accept");
		}

		inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);		//将网络地址转换成“.”点隔的字符串格式
		printf("----------------------------------------------\n");
		printf("client ip: %s,port: %d\n", cli_ip, ntohs(client_addr.sin_port));
		addrinform.connfd = connfd;

		/*
		* 在 C 语言中，数组名是数组第一个元素的地址，而不能直接赋值给数组变量。因此，不能将 cli_ip 直接赋值给 addrinform.ip 成员
		* 如果要将 cli_ip 数组中的字符串赋值给 addrinform.ip 数组成员，可以使用 strcpy() 函数
		*/
		strcpy(addrinform.ip, cli_ip);
		addrinform.port = ntohs(client_addr.sin_port);
		if (connfd > 0)
		{
			pthread_create(&thread_id, NULL, client_process, (void*)&addrinform);
			pthread_detach(thread_id);
		}
	}
	close(sockfd);
	return 0;
}