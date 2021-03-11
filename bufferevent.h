
#ifndef EVENT2_BUFFEREVENT_H_INCLUDED_
#define EVENT2_BUFFEREVENT_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"

//bev错误回调码
#define BEV_EVENT_READING	0x01	
#define BEV_EVENT_WRITING	0x02	
#define BEV_EVENT_EOF		0x10	
#define BEV_EVENT_ERROR		0x20	
#define BEV_EVENT_TIMEOUT	0x40	
#define BEV_EVENT_CONNECTED	0x80	

//用不到
enum bufferevent_flush_mode {
	BEV_NORMAL = 0,
	BEV_FLUSH = 1,
	BEV_FINISHED = 2
};

struct bufferevent;
struct event_base;
struct evbuffer;

typedef void (*bufferevent_data_cb)(struct bufferevent *bev, void *ctx);

typedef void (*bufferevent_event_cb)(struct bufferevent *bev, short what, void *ctx);

enum bufferevent_options {
	BEV_OPT_CLOSE_ON_FREE = (1<<0),

	BEV_OPT_THREADSAFE = (1<<1),

	BEV_OPT_DEFER_CALLBACKS = (1<<2),

	BEV_OPT_UNLOCK_CALLBACKS = (1<<3)
};

struct bufferevent *bufferevent_socket_new(struct event_base *base, evutil_socket_t fd, int options);

int bufferevent_base_set(struct event_base *base, struct bufferevent *bufev);

struct event_base *bufferevent_get_base(struct bufferevent *bev);

void bufferevent_free(struct bufferevent *bufev);

void bufferevent_setcb(struct bufferevent *bufev,bufferevent_data_cb readcb, bufferevent_data_cb writecb,bufferevent_event_cb eventcb, void *cbarg);

evutil_socket_t bufferevent_getfd(struct bufferevent *bufev);

int bufferevent_write(struct bufferevent *bufev,const void *data, size_t size);

size_t bufferevent_read(struct bufferevent *bufev, void *data, size_t size);

struct evbuffer *bufferevent_get_input(struct bufferevent *bufev);

struct evbuffer *bufferevent_get_output(struct bufferevent *bufev);

int bufferevent_enable(struct bufferevent *bufev, short event);

int bufferevent_disable(struct bufferevent *bufev, short event);

#ifdef __cplusplus
}
#endif

#endif /* EVENT2_BUFFEREVENT_H_INCLUDED_ */
