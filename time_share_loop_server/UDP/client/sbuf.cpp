/*
*要求：
*	将客户端程序部署在windows上
*详解：
*	首先使用库函数WSAStartup()初始化套接字库，然后创建一个套接字，使用sendto()函数向服务器发送数据。
*	使用recvfrom()函数接收服务器的回应，最后使用closesocket()函数关闭套接字，使用WSACleanup()函数终止套接字库的使用。
*/

#include <stdio.h>
#include <winsock.h>
#pragma comment(lib, "wsock32")		//声明引用库
#define BUF_SIZE 200
#define PORT 8888
char wbuf[50], rbuf[50];

int main()
{
	SOCKET s;
	int len;
	WSADATA wsadata;
	struct hostent *phe;				/*host information*/
	struct servent *pse;				/*server information*/
	struct protoent *ppe;				/*protocol information*/
	struct sockaddr_in saddr, raddr;	/*an Internet endpoint address*/

	int fromlen, ret, type;
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

	/**** get protocol number from protocol name ****/
	if ((ppe = getprotobyname("UDP")) == 0)
	{
		printf("getprotobyname failed!\n");
		WSACleanup();
		return -1;
	}

	s = socket(PF_INET, SOCK_DGRAM, ppe->p_proto);
	if (s == INVALID_SOCKET)
	{
		printf("creat socket error\n");
		WSACleanup();
		return -1;
	}

	fromlen = sizeof(struct sockaddr);
	printf("please enter data:");
	scanf_s("%s", wbuf, sizeof(wbuf));

	ret = sendto(s, wbuf, sizeof(wbuf), 0, (struct sockaddr*)&saddr, sizeof(struct sockaddr));
	if (ret < 0)
	{
		perror("sendto faild");
	}

	len = recvfrom(s, rbuf, sizeof(rbuf), 0, (struct sockaddr *)&raddr, &fromlen);
	if (len < 0)
	{
		perror("recvfrom faild");
	}
	
	printf("server reply:%s\n", rbuf);

	closesocket(s);
	WSACleanup();
	return 0;

}
