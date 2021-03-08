

#ifdef _WIN32
#include <windows.h>
#endif

#include <time.h>
#include "util.h"
#include "time-internal.h"

#ifdef _WIN32
int evutil_gettimeofday(struct timeval *tv, struct timezone *tz)
{
#ifdef _MSC_VER
#define U64_LITERAL(n) n##ui64
#else
#define U64_LITERAL(n) n##llu
#endif

#define EPOCH_BIAS U64_LITERAL(116444736000000000)		//116444736000000000Ϊ1601�굽1970�����������windowsʱ���?601�꿪ʼ�㣬��С��λ��100ns��
#define UNITS_PER_SEC U64_LITERAL(10000000)				//ns->us->ms->s
#define USEC_PER_SEC U64_LITERAL(1000000)
#define UNITS_PER_USEC U64_LITERAL(10)

	union {
		FILETIME ft_ft;
		ev_uint64_t ft_64;
	} ft;

	if (tv == NULL)
		return -1;

	GetSystemTimeAsFileTime(&ft.ft_ft);		//��ȡ�����Ǵ�1601��1��1�յ�Ŀǰ����������

	if (ft.ft_64 < EPOCH_BIAS) {
		return -1;
	}
	ft.ft_64 -= EPOCH_BIAS;
	tv->tv_sec = (long)(ft.ft_64 / UNITS_PER_SEC);
	tv->tv_usec = (long)((ft.ft_64 / UNITS_PER_USEC) % USEC_PER_SEC);
	return 0;
}
#endif // _WIN32

#define MAX_SECONDS_IN_MSEC_LONG \
	(((LONG_MAX)-999) / 1000)
long evutil_tv_to_msec_(const struct timeval *tv)
{
	if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
		return -1;

	return (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
}

static void adjust_monotonic_time(struct evutil_monotonic_timer *base,struct timeval *tv)
{//�������ص���ʱ��ƫ�ƣ���֤ʱ�䵥����
	evutil_timeradd(tv, &base->adjust_monotonic_clock, tv);

	if (evutil_timercmp(tv, &base->last_time, < )) {
		/* Guess it wasn't monotonic after all. */
		struct timeval adjust;
		evutil_timersub(&base->last_time, tv, &adjust);
		evutil_timeradd(&adjust, &base->adjust_monotonic_clock,
			&base->adjust_monotonic_clock);
		*tv = base->last_time;
	}
	base->last_time = *tv;
}

#ifdef _WIN32
static ev_uint64_t evutil_GetTickCount_(struct evutil_monotonic_timer *base)
{
	if (base->GetTickCount64_fn) {
		return base->GetTickCount64_fn();
	}
	else if (base->GetTickCount_fn) {
		ev_uint64_t v = base->GetTickCount_fn();
		return (DWORD)v | ((v >> 18) & 0xFFFFFFFF00000000);
	}
	else {
		DWORD ticks = GetTickCount();
		if (ticks < base->last_tick_count) {
			base->adjust_tick_count += ((ev_uint64_t)1) << 32;
		}
		base->last_tick_count = ticks;
		return ticks + base->adjust_tick_count;
	}
}

#ifdef _WIN32
#include <tchar.h>
HMODULE evutil_load_windows_system_library_(const TCHAR *library_name)
{
	TCHAR path[MAX_PATH];
	unsigned n;
	n = GetSystemDirectory(path, MAX_PATH);
	if (n == 0 || n + _tcslen(library_name) + 2 >= MAX_PATH)
		return 0;
	_tcscat(path, TEXT("\\"));
	_tcscat(path, library_name);
	return LoadLibrary(path);
}
#endif

int evutil_configure_monotonic_time_(struct evutil_monotonic_timer *base,int flags)
{
	const int precise = flags & EV_MONOT_PRECISE;
	const int fallback = flags & EV_MONOT_FALLBACK;
	HANDLE h;
	memset(base, 0, sizeof(*base));

	h = evutil_load_windows_system_library_(TEXT("kernel32.dll"));
	if (h != NULL && !fallback) {
		base->GetTickCount64_fn = (ev_GetTickCount_func)GetProcAddress(h, "GetTickCount64");
		base->GetTickCount_fn = (ev_GetTickCount_func)GetProcAddress(h, "GetTickCount");
	}

	base->first_tick = base->last_tick_count = evutil_GetTickCount_(base);
	if (precise && !fallback) {
		LARGE_INTEGER freq;
		if (QueryPerformanceFrequency(&freq)) {
			LARGE_INTEGER counter;
			QueryPerformanceCounter(&counter);
			base->first_counter = counter.QuadPart;
			base->usec_per_count = 1.0e6 / freq.QuadPart;
			base->use_performance_counter = 1;
		}
	}

	return 0;
}

static ev_int64_t abs64(ev_int64_t i)
{
	return i < 0 ? -i : i;
}

int evutil_gettime_monotonic_(struct evutil_monotonic_timer *base,struct timeval *tp)
{
	ev_uint64_t ticks = evutil_GetTickCount_(base);
	if (base->use_performance_counter) {
		LARGE_INTEGER counter;
		ev_int64_t counter_elapsed, counter_usec_elapsed, ticks_elapsed;
		QueryPerformanceCounter(&counter);
		counter_elapsed = (ev_int64_t)
			(counter.QuadPart - base->first_counter);
		ticks_elapsed = ticks - base->first_tick;
		counter_usec_elapsed = (ev_int64_t)
			(counter_elapsed * base->usec_per_count);

		//���QueryPerformanceCounter��GetTickCount������һ�룬����GetTickCountʱ��Ϊ׼��������QueryPerformanceCounterΪ׼
		if (abs64(ticks_elapsed * 1000 - counter_usec_elapsed) > 1000000) {
			counter_usec_elapsed = ticks_elapsed * 1000;
			base->first_counter = (ev_uint64_t)(counter.QuadPart - counter_usec_elapsed / base->usec_per_count);
		}
		tp->tv_sec = (time_t)(counter_usec_elapsed / 1000000);
		tp->tv_usec = counter_usec_elapsed % 1000000;

	}
	else {
		/* We're just using GetTickCount(). */
		tp->tv_sec = (time_t)(ticks / 1000);
		tp->tv_usec = (ticks % 1000) * 1000;
	}
	adjust_monotonic_time(base, tp);

	return 0;
}

#endif // _WIN32


#ifndef _WIN32

int evutil_configure_monotonic_time_(struct evutil_monotonic_timer *base,
    int flags)
{

#ifdef CLOCK_MONOTONIC_COARSE
	const int precise = flags & EV_MONOT_PRECISE;
#endif
	const int fallback = flags & EV_MONOT_FALLBACK;
	struct timespec	ts;

#ifdef CLOCK_MONOTONIC_COARSE
	if (CLOCK_MONOTONIC_COARSE < 0) {
		//event_errx(1,"I didn't expect CLOCK_MONOTONIC_COARSE to be < 0");
	}
	if (! precise && ! fallback) {
		if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) == 0) {
			base->monotonic_clock = CLOCK_MONOTONIC_COARSE;
			return 0;
		}
	}
#endif
	if (!fallback && clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		base->monotonic_clock = CLOCK_MONOTONIC;
		return 0;
	}

	if (CLOCK_MONOTONIC < 0) {
		//event_errx(1,"I didn't expect CLOCK_MONOTONIC to be < 0");
	}

	base->monotonic_clock = -1;
	return 0;
}

int
evutil_gettime_monotonic_(struct evutil_monotonic_timer *base,
    struct timeval *tp)
{
	struct timespec ts;

	if (base->monotonic_clock < 0) {
		if (evutil_gettimeofday(tp, NULL) < 0)
			return -1;
		adjust_monotonic_time(base, tp);
		return 0;
	}

	if (clock_gettime(base->monotonic_clock, &ts) == -1)
		return -1;
	tp->tv_sec = ts.tv_sec;
	tp->tv_usec = ts.tv_nsec / 1000;

	return 0;
}
#endif