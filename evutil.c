
#include "util.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<fcntl.h>
#endif

int
evutil_make_socket_nonblocking(evutil_socket_t fd)
{
#ifdef _WIN32
	{
		unsigned long nonblocking = 1;
		if (ioctlsocket(fd, FIONBIO, &nonblocking) == SOCKET_ERROR) {
			return -1;
		}
	}
#else
	{
		int flags;
		if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
			return -1;
		}
		if (!(flags & O_NONBLOCK)) {
			if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
				return -1;
			}
		}
	}
#endif
	return 0;
}

int
evutil_make_listen_socket_reuseable(evutil_socket_t sock)
{
#if defined(SO_REUSEADDR) && !defined(_WIN32)
	int one = 1;
	/* REUSEADDR on Unix means, "don't hang on to this address after the
	 * listener is closed."  On Windows, though, it means "don't keep other
	 * processes from binding to this address while we're using it. */
	return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*) &one,
	    (ev_socklen_t)sizeof(one));
#else
	return 0;
#endif
}

int
evutil_closesocket(evutil_socket_t sock)
{
#ifndef _WIN32
	return close(sock);
#else
	return closesocket(sock);
#endif
}


