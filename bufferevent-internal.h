
#ifndef BUFFEREVENT_INTERNAL_H_INCLUDED_
#define BUFFEREVENT_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

// #include "event2/event-config.h"
// #include "event2/event_struct.h"
// #include "evconfig-private.h"
#include "util.h"
// #include "defer-internal.h"
// #include "evthread-internal.h"
// #include "event2/thread.h"
// #include "ratelim-internal.h"
#include "bufferevent_struct.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#endif


typedef ev_uint16_t bufferevent_suspend_flags;

struct bufferevent_private {
	struct bufferevent bev;
	ev_ssize_t max_single_read;
	ev_ssize_t max_single_write;
	enum bufferevent_options options;
};

enum bufferevent_ctrl_op {
	BEV_CTRL_SET_FD,
	BEV_CTRL_GET_FD,
	BEV_CTRL_GET_UNDERLYING,
	BEV_CTRL_CANCEL_ALL
};

/** Possible data types for a control callback */
union bufferevent_ctrl_data {
	void *ptr;
	evutil_socket_t fd;
};

struct bufferevent_ops {
	const char *type;
	int mem_offset;
	int (*enable)(struct bufferevent *, short);
	int (*disable)(struct bufferevent *, short);
	void (*unlink)(struct bufferevent *);
	void (*destruct)(struct bufferevent *);
	int (*adj_timeouts)(struct bufferevent *);
	int (*flush)(struct bufferevent *, short, enum bufferevent_flush_mode);
	int (*ctrl)(struct bufferevent *, enum bufferevent_ctrl_op, union bufferevent_ctrl_data *);
};

extern const struct bufferevent_ops bufferevent_ops_socket;

int bufferevent_init_common_(struct bufferevent_private *, struct event_base *, const struct bufferevent_ops *, enum bufferevent_options options);

void bufferevent_run_readcb_(struct bufferevent *bufev, int options);

void bufferevent_run_writecb_(struct bufferevent *bufev, int options);

void bufferevent_run_eventcb_(struct bufferevent *bufev, short what, int options);

static void bufferevent_trigger_nolock_(struct bufferevent *bufev, short iotype, int options);

int bufferevent_add_event_(struct event *ev, const struct timeval *tv);

ev_ssize_t bufferevent_get_read_max_(struct bufferevent_private *bev);

ev_ssize_t bufferevent_get_write_max_(struct bufferevent_private *bev);

int bufferevent_generic_adj_existing_timeouts_(struct bufferevent *bev);

int bufferevent_ratelim_init_(struct bufferevent_private *bev);

int bufferevent_decref_and_unlock_(struct bufferevent *bufev);

#ifdef __cplusplus
}
#endif


#endif /* BUFFEREVENT_INTERNAL_H_INCLUDED_ */
