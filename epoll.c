
#include "event-config.h"

#ifdef EVENT__HAVE_EPOLL

#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include "event-internal.h"
#include "evmap-internal.h"
#include "changelist-internal.h"
#include "mm-internal.h"
#include "epolltable-internal.h"

struct epollop {
	struct epoll_event *events;
	int nevents;
	int epfd;
// #ifdef USING_TIMERFD
// 	int timerfd;
// #endif
};

static void *epoll_init(struct event_base *);
static int epoll_dispatch(struct event_base *, struct timeval *);
static void epoll_dealloc(struct event_base *);

static int epoll_nochangelist_add(struct event_base *base, evutil_socket_t fd,short old, short events, void *p);
static int epoll_nochangelist_del(struct event_base *base, evutil_socket_t fd,short old, short events, void *p);

const struct eventop epollops = {
	"epoll",
	epoll_init,
	epoll_nochangelist_add,
	epoll_nochangelist_del,
	epoll_dispatch,
	epoll_dealloc,
	1, /* need reinit */
	EV_FEATURE_ET|EV_FEATURE_O1|EV_FEATURE_EARLY_CLOSE,
	0
};

#define INITIAL_NEVENT 32
#define MAX_NEVENT 4096

#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

static void *
epoll_init(struct event_base *base)
{
	int epfd = -1;
	struct epollop *epollop;

// #ifdef EVENT__HAVE_EPOLL_CREATE1
// 	/* First, try the shiny new epoll_create1 interface, if we have it. */
// 	epfd = epoll_create1(EPOLL_CLOEXEC);
// #endif
	if (epfd == -1) {
		if ((epfd = epoll_create(32000)) == -1) {
			return (NULL);
		}
		// evutil_make_socket_closeonexec(epfd);
	}

	if (!(epollop = mm_calloc(1, sizeof(struct epollop)))) {
		close(epfd);
		return (NULL);
	}

	epollop->epfd = epfd;

	/* Initialize fields */
	epollop->events = mm_calloc(INITIAL_NEVENT, sizeof(struct epoll_event));
	if (epollop->events == NULL) {
		mm_free(epollop);
		close(epfd);
		return (NULL);
	}
	epollop->nevents = INITIAL_NEVENT;

	return (epollop);
}

static int epoll_apply_one_change(struct event_base *base,struct epollop *epollop,const struct event_change *ch)
{
	struct epoll_event epev;
	int op, events = 0;
	int idx;

	idx = EPOLL_OP_TABLE_INDEX(ch);
	op = epoll_op_table[idx].op;
	events = epoll_op_table[idx].events;

	if (!events) {
		return 0;
	}

	if ((ch->read_change|ch->write_change) & EV_CHANGE_ET)
		events |= EPOLLET;

	memset(&epev, 0, sizeof(epev));
	epev.data.fd = ch->fd;
	epev.events = events;
	if (epoll_ctl(epollop->epfd, op, ch->fd, &epev) == 0) {
		return 0;
	}

	switch (op) {
	case EPOLL_CTL_MOD:
		//op是EPOLL_CTL_MOD或EPOLL_CTL_DEL，并且fd未在此epoll实例中注册
		if (errno == ENOENT) {
			if (epoll_ctl(epollop->epfd, EPOLL_CTL_ADD, ch->fd, &epev) == -1) {
				return -1;
			} else {
				return 0;
			}
		}
		break;
	case EPOLL_CTL_ADD:
		//op是EPOLL_CTL_ADD，并且提供的文件描述符fd已经在此epoll实例中注册
		if (errno == EEXIST) {
			if (epoll_ctl(epollop->epfd, EPOLL_CTL_MOD, ch->fd, &epev) == -1) {
				return -1;
			} else {
				return 0;
			}
		}
		break;
	case EPOLL_CTL_DEL:
		//other
		if (errno == ENOENT || errno == EBADF || errno == EPERM) {
			return 0;
		}
		break;
	default:
		break;
	}
	return -1;
}

static int epoll_nochangelist_add(struct event_base *base, evutil_socket_t fd,short old, short events, void *p)
{
	struct event_change ch;
	ch.fd = fd;
	ch.old_events = old;
	ch.read_change = ch.write_change = ch.close_change = 0;
	if (events & EV_WRITE)
		ch.write_change = EV_CHANGE_ADD |
		    (events & EV_ET);
	if (events & EV_READ)
		ch.read_change = EV_CHANGE_ADD |
		    (events & EV_ET);
	if (events & EV_CLOSED)
		ch.close_change = EV_CHANGE_ADD |
		    (events & EV_ET);

	return epoll_apply_one_change(base, base->evbase, &ch);
}

static int epoll_nochangelist_del(struct event_base *base, evutil_socket_t fd,short old, short events, void *p)
{
	struct event_change ch;
	ch.fd = fd;
	ch.old_events = old;
	ch.read_change = ch.write_change = ch.close_change = 0;
	if (events & EV_WRITE)
		ch.write_change = EV_CHANGE_DEL |
		    (events & EV_ET);
	if (events & EV_READ)
		ch.read_change = EV_CHANGE_DEL |
		    (events & EV_ET);
	if (events & EV_CLOSED)
		ch.close_change = EV_CHANGE_DEL |
		    (events & EV_ET);

	return epoll_apply_one_change(base, base->evbase, &ch);
}

static int epoll_dispatch(struct event_base *base, struct timeval *tv)
{
	struct epollop *epollop = base->evbase;
	struct epoll_event *events = epollop->events;
	int i, res;
	long timeout = -1;

	if (tv != NULL) {
		//timeout = evutil_tv_to_msec_(tv);
		timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
		if (timeout < 0 || timeout > MAX_EPOLL_TIMEOUT_MSEC) {
			timeout = MAX_EPOLL_TIMEOUT_MSEC;
		}
	}

	res = epoll_wait(epollop->epfd, events, epollop->nevents, timeout);

	if (res == -1) {
		if (errno != EINTR) {
			return (-1);
		}
		return (0);
	}

	for (i = 0; i < res; i++) {
		int what = events[i].events;
		short ev = 0;

		if (what & (EPOLLHUP|EPOLLERR)) {
			ev = EV_READ | EV_WRITE;
		} else {
			if (what & EPOLLIN)
				ev |= EV_READ;
			if (what & EPOLLOUT)
				ev |= EV_WRITE;
			if (what & EPOLLRDHUP)
				ev |= EV_CLOSED;
		}

		if (!ev)
			continue;

		evmap_io_active_(base, events[i].data.fd, ev | EV_ET);
	}

	if (res == epollop->nevents && epollop->nevents < MAX_NEVENT) {
		int new_nevents = epollop->nevents * 2;
		struct epoll_event *new_events;

		new_events = mm_realloc(epollop->events,
		    new_nevents * sizeof(struct epoll_event));
		if (new_events) {
			epollop->events = new_events;
			epollop->nevents = new_nevents;
		}
	}

	return (0);
}

static void epoll_dealloc(struct event_base *base)
{
	struct epollop *epollop = base->evbase;

	if (epollop->events)
		mm_free(epollop->events);
	if (epollop->epfd >= 0)
		close(epollop->epfd);

	memset(epollop, 0, sizeof(struct epollop));
	mm_free(epollop);
}

#endif /* EVENT__HAVE_EPOLL */
