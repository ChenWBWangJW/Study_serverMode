#include <stdio.h>
#include <Winsock.h>
#include <errno.h>
#pragma comment(lib, "ws2_32.lib");

#define PORT 8888
#define BUF_LEN 200

typedef struct sockaddr_in SAIN;
typedef struct sockaddr SA;

void perrorText(const char* str)
{
	fprintf(stderr, "%s error. reason code: %d\n", str, WSAGetLastError());
	exit(-1);
}

int main(void)
{
	char buf[BUF_LEN];
	
	//1.初始化winsock库
	/************************************
	* 函数名：int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
	* 功能：初始化 Winsock 库。在使用 Winsock 库进行网络编程之前，必须先调用 WSAStartup() 函数对 Winsock 库进行初始化
	* 输入：
	*	wVersionRequested - 期望使用的winsock版本，一般设置为 MAKEWORD(2, 2)，表示使用 Winsock2.2 版本
	* 
	*	lpWSAData - 指向WSADATA结构的指针，该结构用来接收winsock函数的细节
	* 
	* 返回值：
	*	0 - 成功
	*	非零 = 失败，WSAGetLastError获取errno
	* 
	* 注意:
	*	每次调用 WSAStartup() 函数后，都必须调用 WSACleanup() 函数来释放 Winsock 库占用的资源
	*************************************/
	WSADATA wasdata;
	if ((WSAStartup(MAKEWORD(2, 2), &wasdata)) != 0)
		perrorText("WSAStartup");

	//2.创建socket
	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == INVALID_SOCKET)
		perrorText("socket");

	//3.配置地址结构体
	SAIN servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr("192.168.222.3");

	//4.连接服务器
	printf("Connecting to server...\n");
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR)
		perrorText("connect");
	printf("Connected.\n");

	//5.输入待发送数据
	printf("Input message: ");
	gets_s(buf, BUF_LEN);
	int len = strlen(buf);
	buf[len] = '\0';

	//6.发送数据
	printf("Sending message...\n");
	if (send(sockfd, buf, len, 0) == SOCKET_ERROR)
		perrorText("send");
	memset(buf, 0, BUF_LEN);		// Clear buffer For Recv(optional

	//7.接收数据
	printf("Receiving message...\n");
	if (recv(sockfd, buf, BUF_LEN, 0) == SOCKET_ERROR)
		perrorText("recv");

	printf("Received message: %s\n", buf);

	//8.关闭socket
	closesocket(sockfd);

	//9.释放winsock库
	WSACleanup();

	return 0;
}
