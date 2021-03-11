
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "util.h"
#include "buffer.h"
#include "bufferevent.h"
#include "bufferevent_struct.h"
#include "event.h"
#include "event-internal.h"
#include "mm-internal.h"
#include "bufferevent-internal.h"
#include "evbuffer-internal.h"


static void bufferevent_cancel_all_(struct bufferevent *bev);
static void bufferevent_finalize_cb_(struct event_callback *evcb, void *arg_);

void bufferevent_run_readcb_(struct bufferevent *bufev, int options)
{//run user readcb
	struct bufferevent_private *p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	if (bufev->readcb == NULL)
		return;
	bufev->readcb(bufev, bufev->cbarg);
}

void bufferevent_run_writecb_(struct bufferevent *bufev, int options)
{//run user writecb
	struct bufferevent_private *p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	if (bufev->writecb == NULL)
		return;
	bufev->writecb(bufev, bufev->cbarg);
}

void bufferevent_run_eventcb_(struct bufferevent *bufev, short what, int options)
{//run user errorcb
	struct bufferevent_private *p = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	if (bufev->errorcb == NULL)
		return;
	bufev->errorcb(bufev, what, bufev->cbarg);
}

int bufferevent_init_common_(struct bufferevent_private *bufev_private,struct event_base *base,const struct bufferevent_ops *ops,enum bufferevent_options options)
{
	struct bufferevent *bufev = &bufev_private->bev;

	if (!bufev->input) {
		if ((bufev->input = evbuffer_new()) == NULL)
			goto err;
	}

	if (!bufev->output) {
		if ((bufev->output = evbuffer_new()) == NULL)
			goto err;
	}

	bufev->ev_base = base;

	evutil_timerclear(&bufev->timeout_read);
	evutil_timerclear(&bufev->timeout_write);

	bufev->be_ops = ops;

	if (bufferevent_ratelim_init_(bufev_private))
		goto err;

	bufev->enabled = EV_WRITE;
	bufev_private->options = options;

	return 0;

err:
	if (bufev->input) {
		evbuffer_free(bufev->input);
		bufev->input = NULL;
	}
	if (bufev->output) {
		evbuffer_free(bufev->output);
		bufev->output = NULL;
	}
	return -1;
}

void bufferevent_setcb(struct bufferevent *bufev,bufferevent_data_cb readcb, bufferevent_data_cb writecb,bufferevent_event_cb eventcb, void *cbarg)
{
	bufev->readcb = readcb;
	bufev->writecb = writecb;
	bufev->errorcb = eventcb;

	bufev->cbarg = cbarg;
}


struct evbuffer *bufferevent_get_input(struct bufferevent *bufev)
{
	return bufev->input;
}

struct evbuffer *bufferevent_get_output(struct bufferevent *bufev)
{
	return bufev->output;
}

struct event_base *bufferevent_get_base(struct bufferevent *bufev)
{
	return bufev->ev_base;
}

int bufferevent_write(struct bufferevent *bufev, const void *data, size_t size)
{
	if (evbuffer_add(bufev->output, data, size) == -1)
		return (-1);

	return 0;
}

size_t bufferevent_read(struct bufferevent *bufev, void *data, size_t size)
{
	return (evbuffer_remove(bufev->input, data, size));
}

int bufferevent_enable(struct bufferevent *bufev, short event)
{
	struct bufferevent_private *bufev_private = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	short impl_events = event;
	int r = 0;

	bufev->enabled |= event;

	if (impl_events && bufev->be_ops->enable(bufev, impl_events) < 0)
		r = -1;

	return r;
}

int bufferevent_disable(struct bufferevent *bufev, short event)
{
	int r = 0;

	bufev->enabled &= ~event;

	if (bufev->be_ops->disable(bufev, event) < 0)
		r = -1;
	return r;
}

void bufferevent_free(struct bufferevent *bufev)
{
	bufferevent_setcb(bufev, NULL, NULL, NULL, NULL);
	bufferevent_cancel_all_(bufev);
	bufferevent_decref_and_unlock_(bufev);
}

evutil_socket_t bufferevent_getfd(struct bufferevent *bev)
{
	union bufferevent_ctrl_data d;
	int res = -1;
	d.fd = -1;
	if (bev->be_ops->ctrl)
		res = bev->be_ops->ctrl(bev, BEV_CTRL_GET_FD, &d);
	return (res<0) ? -1 : d.fd;
}

static void bufferevent_cancel_all_(struct bufferevent *bev)
{
	union bufferevent_ctrl_data d;
	memset(&d, 0, sizeof(d));
	if (bev->be_ops->ctrl)
		bev->be_ops->ctrl(bev, BEV_CTRL_CANCEL_ALL, &d);
}

int bufferevent_generic_adj_existing_timeouts_(struct bufferevent *bev)
{
	int r = 0;
	return r;
}

int bufferevent_decref_and_unlock_(struct bufferevent *bufev)
{
	struct bufferevent_private *bufev_private = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);
	int n_cbs = 0;
#define MAX_CBS 16
	struct event_callback *cbs[MAX_CBS];

	if (bufev->be_ops->unlink)
		bufev->be_ops->unlink(bufev);

	cbs[0] = &bufev->ev_read.ev_evcallback;
	cbs[1] = &bufev->ev_write.ev_evcallback;
	n_cbs = 2;

	event_callback_finalize_many_(bufev->ev_base, n_cbs, cbs,bufferevent_finalize_cb_);

#undef MAX_CBS
	return 1;
}

static void bufferevent_finalize_cb_(struct event_callback *evcb, void *arg_)
{
	struct bufferevent *bufev = arg_;
	//struct bufferevent *underlying;
	struct bufferevent_private *bufev_private = EVUTIL_UPCAST((bufev), struct bufferevent_private, bev);

	if (bufev->be_ops->destruct)
		bufev->be_ops->destruct(bufev);

	evbuffer_free(bufev->input);
	evbuffer_free(bufev->output);

	mm_free(((char*)bufev) - bufev->be_ops->mem_offset);
}