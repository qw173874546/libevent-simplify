
#ifndef EVENT2_BUFFEREVENT_STRUCT_H_INCLUDED_
#define EVENT2_BUFFEREVENT_STRUCT_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"
#include "event_struct.h"

struct event_watermark {
	size_t low;
	size_t high;
};

struct bufferevent {
	struct event_base *ev_base;
	const struct bufferevent_ops *be_ops;

	struct event ev_read;
	struct event ev_write;

	struct evbuffer *input;

	struct evbuffer *output;

	struct event_watermark wm_read;
	struct event_watermark wm_write;

	bufferevent_data_cb readcb;
	bufferevent_data_cb writecb;
	bufferevent_event_cb errorcb;
	void *cbarg;

	struct timeval timeout_read;
	struct timeval timeout_write;

	short enabled;
};

#ifdef __cplusplus
}
#endif

#endif /* EVENT2_BUFFEREVENT_STRUCT_H_INCLUDED_ */
