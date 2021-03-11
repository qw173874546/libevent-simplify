
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif


#include "util.h"
#include "bufferevent.h"
#include "buffer.h"
#include "bufferevent_struct.h"
#include "event.h"
#include "mm-internal.h"
#include "bufferevent-internal.h"


static int be_socket_enable(struct bufferevent *, short);
static int be_socket_disable(struct bufferevent *, short);
static void be_socket_destruct(struct bufferevent *);
static int be_socket_flush(struct bufferevent *, short, enum bufferevent_flush_mode);
static int be_socket_ctrl(struct bufferevent *, enum bufferevent_ctrl_op, union bufferevent_ctrl_data *);

static void be_socket_setfd(struct bufferevent *, evutil_socket_t);

const struct bufferevent_ops bufferevent_ops_socket = {
	"socket",
	offsetof(struct bufferevent_private, bev),
	be_socket_enable,
	be_socket_disable,
	NULL, /* unlink */
	be_socket_destruct,
	bufferevent_generic_adj_existing_timeouts_,
	be_socket_flush,
	be_socket_ctrl,
};

static void bufferevent_socket_outbuf_cb(struct evbuffer *buf,const struct evbuffer_cb_info *cbinfo,void *arg)
{//output如果添加了内容，则添加event
	struct bufferevent *bufev = arg;
	struct bufferevent_private *bufev_p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);

	if (cbinfo->n_added && (bufev->enabled & EV_WRITE)) {
		if (bufferevent_add_event_(&bufev->ev_write, &bufev->timeout_write) == -1) {
			/* Should we log this? */
		}
	}
}

static void bufferevent_readcb(evutil_socket_t fd, short event, void *arg)
{//io readcb
	struct bufferevent *bufev = arg;
	struct bufferevent_private *bufev_p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	struct evbuffer *input;
	int res = 0;
	short what = BEV_EVENT_READING;
	ev_ssize_t howmuch = -1, readmax=-1;

	if (event == EV_TIMEOUT) {
		what |= BEV_EVENT_TIMEOUT;
		goto error;
	}

	input = bufev->input;

	if (bufev->wm_read.high != 0) {
		howmuch = bufev->wm_read.high - evbuffer_get_length(input);
		if (howmuch <= 0) {
			goto done;
		}
	}
	readmax = bufferevent_get_read_max_(bufev_p);
	if (howmuch < 0 || howmuch > readmax) 
		howmuch = readmax;

	res = evbuffer_read(input, fd, (int)howmuch); 

	if (res == -1) {
		what |= BEV_EVENT_ERROR;
	} else if (res == 0) {
		what |= BEV_EVENT_EOF;
	}

	if (res <= 0)
		goto error;

	/* Invoke the user callback - must always be called last */
	bufferevent_trigger_nolock_(bufev, EV_READ, 0);
	
	goto done;
 error:
	bufferevent_disable(bufev, EV_READ);
	bufferevent_run_eventcb_(bufev, what, 0);
 done:
	return;
}

static void bufferevent_writecb(evutil_socket_t fd, short event, void *arg)
{//把output内容发送，如果output发送完则移除event
	struct bufferevent *bufev = arg;
	struct bufferevent_private *bufev_p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	int res = 0;
	short what = BEV_EVENT_WRITING;
	int connected = 0;
	ev_ssize_t atmost = -1;

	if (event == EV_TIMEOUT) {
		what |= BEV_EVENT_TIMEOUT;
		goto error;
	}

	atmost = bufferevent_get_write_max_(bufev_p);

	if (evbuffer_get_length(bufev->output)) {
		res = evbuffer_write_atmost(bufev->output, fd, atmost);
		if (res == -1) {
			what |= BEV_EVENT_ERROR;
		} else if (res == 0) {
			what |= BEV_EVENT_EOF;
		}
		if (res <= 0)
			goto error;
	}

	if (evbuffer_get_length(bufev->output) == 0) {
		event_del(&bufev->ev_write);
	}

	if (res ) {
		bufferevent_trigger_nolock_(bufev, EV_WRITE, 0);
	}

	goto done;
 error:
	bufferevent_disable(bufev, EV_WRITE);
	bufferevent_run_eventcb_(bufev, what, 0);
done:
	return;
}

struct bufferevent *bufferevent_socket_new(struct event_base *base, evutil_socket_t fd,int options)
{
	struct bufferevent_private *bufev_p;
	struct bufferevent *bufev;
	
	if ((bufev_p = mm_calloc(1, sizeof(struct bufferevent_private)))== NULL)
		return NULL;

	if (bufferevent_init_common_(bufev_p, base, &bufferevent_ops_socket,options) < 0) {
		mm_free(bufev_p);
		return NULL;
	}
	bufev = &bufev_p->bev;

	event_assign(&bufev->ev_read, bufev->ev_base, fd, EV_READ|EV_PERSIST, bufferevent_readcb, bufev);
	event_assign(&bufev->ev_write, bufev->ev_base, fd, EV_WRITE|EV_PERSIST, bufferevent_writecb, bufev);

	evbuffer_add_cb(bufev->output, bufferevent_socket_outbuf_cb, bufev);
	return bufev;
}


static int be_socket_enable(struct bufferevent *bufev, short event)
{
	if (event & EV_READ && bufferevent_add_event_(&bufev->ev_read, &bufev->timeout_read) == -1)
		return -1;
	if (event & EV_WRITE && bufferevent_add_event_(&bufev->ev_write, &bufev->timeout_write) == -1)
		return -1;
	return 0;
}

static int be_socket_disable(struct bufferevent *bufev, short event)
{
	struct bufferevent_private *bufev_p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	if (event & EV_READ) {
		if (event_del(&bufev->ev_read) == -1)
			return -1;
	}

	if ((event & EV_WRITE)) {
		if (event_del(&bufev->ev_write) == -1)
			return -1;
	}
	return 0;
}

static void be_socket_destruct(struct bufferevent *bufev)
{
	struct bufferevent_private *bufev_p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	evutil_socket_t fd;

	fd = event_get_fd(&bufev->ev_read);

	if ((bufev_p->options & BEV_OPT_CLOSE_ON_FREE) && fd >= 0)
		evutil_closesocket(fd);
}

static int be_socket_flush(struct bufferevent *bev, short iotype,enum bufferevent_flush_mode mode)
{
	return 0;
}


static void be_socket_setfd(struct bufferevent *bufev, evutil_socket_t fd)
{
	struct bufferevent_private *bufev_p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);

	event_del(&bufev->ev_read);
	event_del(&bufev->ev_write);

	event_assign(&bufev->ev_read, bufev->ev_base, fd,EV_READ|EV_PERSIST, bufferevent_readcb, bufev);
	event_assign(&bufev->ev_write, bufev->ev_base, fd,EV_WRITE|EV_PERSIST, bufferevent_writecb, bufev);

	if (fd >= 0)
		bufferevent_enable(bufev, bufev->enabled);
}

static int be_socket_ctrl(struct bufferevent *bev, enum bufferevent_ctrl_op op,union bufferevent_ctrl_data *data)
{
	switch (op) {
	case BEV_CTRL_SET_FD:
		be_socket_setfd(bev, data->fd);
		return 0;
	case BEV_CTRL_GET_FD:
		data->fd = event_get_fd(&bev->ev_read);
		return 0;
	case BEV_CTRL_GET_UNDERLYING:
	case BEV_CTRL_CANCEL_ALL:
	default:
		return -1;
	}
}

int bufferevent_add_event_(struct event *ev, const struct timeval *tv)
{
	if (!evutil_timerisset(tv))
		return event_add(ev, NULL);
	else
		return event_add(ev, tv);
}

static void bufferevent_trigger_nolock_(struct bufferevent *bufev, short iotype, int options)
{
	if ((iotype & EV_READ) && ( evbuffer_get_length(bufev->input) >= bufev->wm_read.low))
		bufferevent_run_readcb_(bufev, options);
	if ((iotype & EV_WRITE) && (evbuffer_get_length(bufev->output) <= bufev->wm_write.low))
		bufferevent_run_writecb_(bufev, options);
}