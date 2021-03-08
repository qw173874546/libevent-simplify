
#ifndef TIME_INTERNAL_H_INCLUDED_
#define TIME_INTERNAL_H_INCLUDED_

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#include <time.h>
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	typedef ULONGLONG(WINAPI *ev_GetTickCount_func)(void);
#endif

long evutil_tv_to_msec_(const struct timeval *tv);

struct evutil_monotonic_timer {

#ifdef HAVE_MACH_MONOTONIC
	struct mach_timebase_info mach_timebase_units;
#endif

#ifndef _WIN32
	int monotonic_clock;
#endif

#ifdef _WIN32
	ev_GetTickCount_func GetTickCount64_fn;
	ev_GetTickCount_func GetTickCount_fn;
	ev_uint64_t last_tick_count;
	ev_uint64_t adjust_tick_count;

	ev_uint64_t first_tick;
	ev_uint64_t first_counter;
	double usec_per_count;
	int use_performance_counter;
#endif

	struct timeval adjust_monotonic_clock;
	struct timeval last_time;
};


int evutil_configure_monotonic_time_(struct evutil_monotonic_timer *mt,int flags);

int evutil_gettime_monotonic_(struct evutil_monotonic_timer *mt, struct timeval *tv);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_INTERNAL_H_INCLUDED_ */