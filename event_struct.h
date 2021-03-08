
#ifndef EVENT2_EVENT_STRUCT_H_INCLUDED_
#define EVENT2_EVENT_STRUCT_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <winsock2.h>
#endif

#define EVLIST_TIMEOUT	    0x01	//用不�?
#define EVLIST_INSERTED	    0x02	//表示在evmap�?
#define EVLIST_SIGNAL	    0x04	//用不�?
#define EVLIST_ACTIVE	    0x08	//表示在activequeues�?
#define EVLIST_INTERNAL	    0x10	//用不�?
#define EVLIST_ACTIVE_LATER 0x20	//用不�?
#define EVLIST_FINALIZING   0x40	//用不�?
#define EVLIST_INIT			0x80	//表示初始状�?
#define EVLIST_ALL          0xff	//包含所有事件状态，用于判断合法性的 

struct event;
struct event_callback {
	struct {
		struct event_callback *tqe_next;	
		struct event_callback **tqe_prev;	
	} evcb_active_next;		//指向下一个激活的回调

 	short evcb_flags;		//回调事件的状态标识：EVLIST_xxx
 	ev_uint8_t evcb_pri;	//回调函数的优先级，越小优先级越高
 	ev_uint8_t evcb_closure;	//回调事件的类型：EV_CLOSURE_xxx，根据不同的类型执行下面不同的回�?
	union {
		void(*evcb_callback)(evutil_socket_t, short, void *);
		void(*evcb_selfcb)(struct event_callback *, void *);
		void(*evcb_evfinalize)(struct event *, void *);
		void(*evcb_cbfinalize)(struct event_callback *, void *);
	} evcb_cb_union;	//不同的回调函�?
 	void *evcb_arg;		//回调参数
};

struct event_base;
struct event {
 	struct event_callback ev_evcallback;	//回调函数，一个event对应一个event_callback，可以互相转�?

	union {
		struct {
			struct event *tqe_next;	/* next element */			\
			struct event **tqe_prev;	/* address of previous next element */	\
		} ev_next_with_common_timeout;
		int min_heap_idx;
	} ev_timeout_pos;

	evutil_socket_t ev_fd;		//fd

 	struct event_base *ev_base;		//base

	union {
		struct {
			struct {
				struct event *le_next;	
				struct event **le_prev;	
			} ev_io_next;	//指向下一个event
			struct timeval ev_timeout;
		} ev_io;
	} ev_;

	short ev_events;	//要监控的事件类型：EV_TIMEOUT,EV_READ,EV_WRITE
	short ev_res;		//触发的事件类�?
	struct timeval ev_timeout;	
};

struct event_dlist {
	struct event *lh_first;	/* first element */
};

#ifdef __cplusplus
}
#endif

#endif /* EVENT2_EVENT_STRUCT_H_INCLUDED_ */