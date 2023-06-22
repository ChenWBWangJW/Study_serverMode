/*******************************
* 需求:
*	实现一个由poll方法实现的并发服务器
* 	服务器能够同时处理多个客户端的请求
* 
* 详解：
*	1.在该代码中，首先创建服务器端套接字
*	  然后绑定监听，static_cast是标准运
*	  算符号，类似于强制类型转换。在while
*	  循环内，调用poll函数执行poll操作，
*	  接着根据fds[0].revents来判断发生了
*	  何种事件，并进行相应的处理。
* 
*	2.对于刚刚接收的连接，只接受读事件。
* 
* 流程：
*	1.创建服务器端套接字
*	2.配置地址
* 	3.绑定监听
* 	4.执行poll操作，等待事件发生
*	5.根据fds[0].revents来判断发生了何种事件，并进行相应的处理，通常0号位是sockfd，表示监听套接字，POLLIN事件表示有新的客户端连接
*	6.如果fds[i].revents & (POLLHUP | POLLRDHUP | POLLNVAL)为真，表示客户端挂起|挂起读|无效请求，将该客户端从监听列表中删除
*	7.如果fds[i].revents & POLLIN为真，表示有客户端发送数据，调用read函数读取数据
* 	8.如果fds[i].revents & POLLOUT为真，表示客户端可写，调用write函数发送数据
********************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct sockaddr SA;
typedef struct sockaddr_in SAIN;

void perrorText(const char* str)
{
	fprintf(stderr, "%s: %s\n", str, strerror(errno));
	exit(-1);
}

//定义发送给客户端的字符串
const char resp[] = "The server reveived you message！\n";

int main(void)
{
	const int port = 8888;
	int sockfd, ret;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	fprintf(stderr, "create socket!\n");
	if (sockfd == -1)
		perrorText("socket");

	int opt = 1;
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) == -1)
		perrorText("setsockopt");
	fprintf(stderr, "socket opt set!\n");

	SAIN servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	socklen_t addrLen = sizeof(servaddr);

	if ((bind(sockfd, (SA*)&servaddr, addrLen)) == -1)
		perrorText("bind");
	fprintf(stderr, "socket binded\n");

	if ((listen(sockfd, 5)) == -1)
		perrorText("listen");
	fprintf(stderr, "socket start listen\n");

	//套接字创建完毕
	//初始化监听列表
	//number of poll fds
	int currentFdNum = 1;

	pollfd* fds = static_cast<pollfd*>(calloc(100, sizeof(pollfd)));		//初始化100个pollfd并动态分配指定数量内存空间并初始化为0，需要搭配free使用
	fds[0].fd = sockfd;			//将监听套接字加入监听列表
	fds[0].events = POLLIN;		//监听读事件
	nfds_t nfds = 1;			//nfds_t是pollfd结构体数组的大小类型，用于记录当前监听的描述符数量
	int timeout = -1;			//timeout = -1, poll will block until event occurs

	fprintf(stderr, "poll init!\n");
	while (1)
	{
		//1.执行poll操作，调用poll函数，等待事件发生
		ret = poll(fds, nfds, timeout);
		fprintf(stderr, "poll returned with ret value: %d\n", ret);
		if (ret == -1)									//返回值为-1，出错
		{
			perrorText("poll");
		}
		else if (ret == 0)								//返回值为0，超时
		{
			fprintf(stderr, "poll timeout!\n");
		}
		else											//返回值大于0, 有事件发生
		{
			//got accept
			fprintf(stderr, "checking fds\n");

			//检查是否有新客户端建立连接，0号位是sockfd，表示监听套接字时，POLLIN事件表示有新的客户端连接
			if (fds[0].revents & POLLIN)		//固定0号位为sockfd，如果有新的客户端连接，POLLIN事件发生
			{
				SAIN cliaddr;
				socklen_t cliaddrLen = sizeof(cliaddr);
				int clientfd = accept(sockfd, (SA*)&cliaddr, &cliaddrLen);
				if (clientfd == -1)
				{
					perrorText("accept");
				}
				fprintf(stderr, "accept a new client: %d\n", clientfd);

				//set non-blocking
				int flags = fcntl(clientfd, F_GETFL, 0);		//对文件描述符进行控制操作,如设置文件描述符为非阻塞

				//设置文件描述符为非阻塞
				if ((fcntl(clientfd, F_SETFL, flags | O_NONBLOCK)) == -1)
				{
					perrorText("fcntl");
				}

				//将新的客户端加入监听列表
				//poll的描述符集， 关心POLLIN事件
				fds[currentFdNum].fd = clientfd;
				fds[currentFdNum].events = (POLLIN | POLLRDHUP);
				nfds++;
				currentFdNum++;
				fprintf(stderr, "add clientfd = %d to poll list\n", currentFdNum);
			}

			//client read & write
			//检查其他描述符事件
			for (int i = 1; i < currentFdNum; i++)		//0号位是sockfd，表示监听套接字，不需要检查
			{
				if (fds[i].revents & (POLLHUP | POLLRDHUP | POLLNVAL))		//内核返回值：挂起|挂起读|无效请求
				{
					//客户端描述符关闭
					//设置events为0，fd = -1, poll将不再监听该描述符
					//set not interested
					fprintf(stderr, "client: %d shutdown\n", i);
					close(fds[i].fd);
					fds[i].events = 0;
					fds[i].fd = -1;
					continue;
				}

				//read
				if (fds[i].revents & POLLIN)		//内核返回值：可读
				{
					char buf[1024] = {};
					while (1)
					{
						//读取数据请求
						ret = read(fds[i].fd, buf, 1024);
						fprintf(stderr, "read on: %d returned with valus %d\n", i, ret);
						if (ret == 0)
						{
							fprintf(stderr, "read returned 0 (EOF) on: %d breaking\n", i);
							break;
						}
						if (ret == -1)
						{
							const int tmpErrno = errno;
							//因为read没有读到换行符，会阻塞，这里认为读取完毕
							//实际需要检查读取数据是否完毕
							if (tmpErrno == EWOULDBLOCK || tmpErrno == EAGAIN)
							{
								fprintf(stderr, "read would block, stop reading\n");
								//read is over
								//http pipe line? need to put resp into a queue
								//可以监听时间里，POLLOUT
								fds[i].events |= POLLOUT;		//按位或操作，将POLLOUT置为1，即表示监听写事件，即可写
								break;
							}
							else
							{
								perrorText("read");
							}
						}
					}
				}

				//write
				if (fds[i].revents & POLLOUT)		//内核返回值：可写
				{
					//写入数据请求
					ret = write(fds[i].fd, resp, sizeof(resp));
					fprintf(stderr, "write on: %d returned with value: %d\n", i, ret);

					//这里需要处理EGAIN & EWOULDBLOCK
					if (ret == -1)
					{
						perrorText("write");
					}
					fds[i].events &= !(POLLOUT);		//按位与操作，将POLLOUT置为0，即表示不再监听写事件
				}
			}
		}
	}
	return 0;
}