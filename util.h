
#ifndef EVENT2_UTIL_H_INCLUDED_
#define EVENT2_UTIL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

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

#define ev_ssize_t int
#define evutil_offsetof(type, field) offsetof(type, field)

#define	evutil_timerclear(tvp)	(tvp)->tv_sec = (tvp)->tv_usec = 0

int evutil_make_socket_nonblocking(evutil_socket_t sock);

int evutil_make_listen_socket_reuseable(evutil_socket_t sock);

int evutil_closesocket(evutil_socket_t sock);

#ifndef _WIN32
#define evutil_gettimeofday(tv, tz) gettimeofday((tv), (tz))
#else
struct timezone;
int evutil_gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#ifdef EVENT__HAVE_TIMERADD
#define evutil_timeradd(tvp, uvp, vvp) timeradd((tvp), (uvp), (vvp))
#define evutil_timersub(tvp, uvp, vvp) timersub((tvp), (uvp), (vvp))
#else
#define evutil_timeradd(tvp, uvp, vvp)					\
do {\
(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
if ((vvp)->tv_usec >= 1000000) {	\
		(vvp)->tv_sec++;				\
		(vvp)->tv_usec -= 1000000;			\
}							\
} while (0)
#define	evutil_timersub(tvp, uvp, vvp)					\
do {\
(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
	if ((vvp)->tv_usec < 0) {		\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
	}							\
} while (0)
#endif /* !EVENT__HAVE_TIMERADD */
#define	evutil_timercmp(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	((tvp)->tv_usec cmp (uvp)->tv_usec) :				\
	((tvp)->tv_sec cmp(uvp)->tv_sec))

#define	evutil_timerisset(tvp)	((tvp)->tv_sec || (tvp)->tv_usec)

#define EVUTIL_UPCAST(ptr, type, field)				\
	((type *)(((char*)(ptr)) - evutil_offsetof(type, field)))

#define EV_MONOT_PRECISE  1
#define EV_MONOT_FALLBACK 2

#ifdef __cplusplus
}
#endif

#endif /* EVENT1_EVUTIL_H_INCLUDED_ */