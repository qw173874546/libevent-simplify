
#ifndef EVENT2_BUFFER_H_INCLUDED_
#define EVENT2_BUFFER_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"


struct evbuffer;

struct evbuffer *evbuffer_new(void);

void evbuffer_free(struct evbuffer *buf);

size_t evbuffer_get_length(const struct evbuffer *buf);

int evbuffer_expand(struct evbuffer *buf, size_t datlen);

int evbuffer_add(struct evbuffer *buf, const void *data, size_t datlen);

int evbuffer_remove(struct evbuffer *buf, void *data, size_t datlen);

ev_ssize_t evbuffer_copyout(struct evbuffer *buf, void *data_out, size_t datlen);

int evbuffer_drain(struct evbuffer *buf, size_t len);

int evbuffer_write(struct evbuffer *buffer, evutil_socket_t fd);

int evbuffer_write_atmost(struct evbuffer *buffer, evutil_socket_t fd,ev_ssize_t howmuch);

int evbuffer_read(struct evbuffer *buffer, evutil_socket_t fd, int howmuch);

struct evbuffer_cb_info {
	size_t orig_size;
	size_t n_added;
	size_t n_deleted;
};

typedef void (*evbuffer_cb_func)(struct evbuffer *buffer, const struct evbuffer_cb_info *info, void *arg);

unsigned char *evbuffer_pullup(struct evbuffer *buf, ev_ssize_t size);

struct evbuffer_ptr {
	ev_ssize_t pos;

	struct {
		void *chain;
		size_t pos_in_chain;
	} internal_;
};

ev_ssize_t evbuffer_copyout_from(struct evbuffer *buf, const struct evbuffer_ptr *pos, void *data_out, size_t datlen);

struct evbuffer_cb_entry *evbuffer_add_cb(struct evbuffer *buffer, evbuffer_cb_func cb, void *cbarg);

#ifdef __cplusplus
}
#endif

#endif /* EVENT2_BUFFER_H_INCLUDED_ */
