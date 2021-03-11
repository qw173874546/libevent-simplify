
#ifdef _WIN32
#include<ws2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<signal.h>
#endif // _WIN32

#include <stdio.h>
#include "event.h"
#include "bufferevent.h"

void bev_read_cb(struct bufferevent *bev, void *ctx)
{
	char tmp[1024] = { 0 };
	bufferevent_read(bev, tmp, 1024);
	printf("bev_read_cb:%s\n", tmp);
}

void bev_write_cb(struct bufferevent *bev, void *ctx)
{
	printf("bev_write_cb\n");
}

void bev_error_cb(struct bufferevent *bev, short what, void *ctx)
{
	int fd = bufferevent_getfd(bev);
	bufferevent_free(bev);
	evutil_closesocket(fd);
	printf("bev_error_cb:%d\n", what);
}

void read_cb(evutil_socket_t fd, short event, void *arg)
{
	char tmp[1024] = { 0 };
	int n = recv(fd, tmp, 1024, 0);
	if (n>0)
	{
		printf("read:%s\n",tmp);
	}
	else
	{
		struct event *ev_read = (struct event *)arg;
		event_free(ev_read);
		evutil_closesocket(fd);
		printf("close:%d\n", fd);
	}
}

void write_cb(evutil_socket_t fd, short event, void *arg)
{
	printf("write_cb\n");
}

void do_accept(evutil_socket_t fd, short event, void *arg)
{
	struct sockaddr_in conn_addr;
	socklen_t add_len = sizeof(conn_addr);
	memset(&conn_addr, 0, sizeof(conn_addr));
	int conn_fd = accept(fd, (struct sockaddr*)&conn_addr, &add_len);

	printf("do_accept:%d\n", conn_fd);

 	struct event_base *base = (struct event_base *)arg;

// 	struct event *ev_read = event_new(base, conn_fd, EV_READ | EV_PERSIST, read_cb, event_self_cbarg());
// 	event_add(ev_read, NULL);

	struct bufferevent *bev = bufferevent_socket_new(base, conn_fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, bev_read_cb, bev_write_cb, bev_error_cb, arg);
	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_PERSIST);
}

void timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	struct timeval tv;
	evutil_gettimeofday(&tv, NULL);
	printf("timeout_cb:s=%ld,ms=%ld\n",tv.tv_sec,tv.tv_usec);
}

int main()
{
#ifdef _WIN32
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
#endif
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(10086);

	if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0)
	{
		printf("bind error!\n");
		return 0;
	}
	printf("bind ok!\n");

	if (listen(listen_fd, 100) != 0)
	{
		printf("listen error!\n");
		return 0;
	}
	printf("listen ok:fd=%d\n", listen_fd);

	struct event_base *base = event_base_new();
	printf("used method:%s\n",event_base_get_method(base));

	struct event *ev_listen = event_new(base, listen_fd, EV_READ | EV_PERSIST, do_accept, base);
	event_add(ev_listen, NULL);

// 	struct event *ev_timer = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, timeout_cb, base);
// 	struct timeval tv;
// 	tv.tv_usec = 0;
// 	tv.tv_sec = 2;
// 	event_add(ev_timer, &tv);

	event_base_dispatch(base);
	event_free(ev_listen);
//	event_free(ev_timer);
	event_base_free(base);

	return 0;
}