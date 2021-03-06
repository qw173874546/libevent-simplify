
#ifndef EVENT2_UTIL_H_INCLUDED_
#define EVENT2_UTIL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <limits.h>

//定义通用数据类型
#define ev_uint64_t unsigned long long
#define ev_int64_t signed long long

#define ev_uint32_t unsigned int
#define ev_int32_t signed int

#define ev_uint16_t unsigned short
#define ev_int16_t  signed short

#define ev_uint8_t unsigned char
#define ev_int8_t signed char

#ifdef _WIN32
#define evutil_socket_t intptr_t
#else
#define evutil_socket_t int
#endif

#ifdef _WIN32
#define ev_socklen_t int
#else
#define ev_socklen_t socklen_t
#endif

#define	evutil_timerclear(tvp)	(tvp)->tv_sec = (tvp)->tv_usec = 0

int evutil_make_socket_nonblocking(evutil_socket_t sock);

int evutil_make_listen_socket_reuseable(evutil_socket_t sock);

int evutil_closesocket(evutil_socket_t sock);

#ifdef __cplusplus
}
#endif

#endif /* EVENT1_EVUTIL_H_INCLUDED_ */