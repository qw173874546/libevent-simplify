
#ifndef EVENT2_EVENT_H_INCLUDED_
#define EVENT2_EVENT_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"

#define EVENT_MAX_PRIORITIES 256
typedef void(*event_callback_fn)(evutil_socket_t, short, void *);

#define EV_TIMEOUT	0x01	//超时事件
#define EV_READ		0x02	//读事�?
#define EV_WRITE	0x04	//写事�?
//#define EV_SIGNAL	0x08	//信号
#define EV_PERSIST	0x10	//持久事件
#define EV_ET		0x20	//ET模式(部分io模型支持)
//#define EV_FINALIZE     0x40
#define EV_CLOSED	0x80	//关闭事件(部分io模型支持)


#define EVLOOP_ONCE	0x01		//阻塞直到有激活事件，处理完所有激活事件才会退�?
#define EVLOOP_NONBLOCK	0x02	//不阻塞，有激活事件会先处理完激活事件，直到没有才会退�?
//#define EVLOOP_NO_EXIT_ON_EMPTY 0x04	

enum event_method_feature {
	EV_FEATURE_ET = 0x01,
	EV_FEATURE_O1 = 0x02,
	EV_FEATURE_FDS = 0x04,
	EV_FEATURE_EARLY_CLOSE = 0x08
};

enum event_base_config_flag {
	EVENT_BASE_FLAG_NOLOCK = 0x01,
	EVENT_BASE_FLAG_IGNORE_ENV = 0x02,
	EVENT_BASE_FLAG_STARTUP_IOCP = 0x04,
	EVENT_BASE_FLAG_NO_CACHE_TIME = 0x08,
	EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST = 0x10,
	EVENT_BASE_FLAG_PRECISE_TIMER = 0x20
};

struct event_base *event_base_new(void);

struct event_config *event_config_new(void);

struct event_base *event_base_new_with_config(const struct event_config *);

void event_config_free(struct event_config *cfg);

void event_base_free(struct event_base *);

int event_config_set_flag(struct event_config *cfg, int flag);

int event_config_avoid_method(struct event_config *cfg, const char *method);

int	event_base_priority_init(struct event_base *, int);

struct event *event_new(struct event_base *, evutil_socket_t, short, event_callback_fn, void *);

int event_assign(struct event *, struct event_base *, evutil_socket_t, short, event_callback_fn, void *);

int event_add(struct event *ev, const struct timeval *timeout);

int event_base_loop(struct event_base *, int);

int event_base_dispatch(struct event_base *);

int event_del(struct event *);

void event_free(struct event *);

void *event_self_cbarg(void);

const char *event_base_get_method(const struct event_base *);

evutil_socket_t event_get_fd(const struct event *ev);

#ifdef __cplusplus
}
#endif

#endif