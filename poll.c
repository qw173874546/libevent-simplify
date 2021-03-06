
#include "event-config.h"

#ifdef EVENT__HAVE_POLL

#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/time.h>

#include "event-internal.h"
#include "evmap-internal.h"
#include "mm-internal.h"

struct pollidx {
	int idxplus1;		//在event_set中数组的索引
};

struct pollop {
	int event_count;		//pollfd数组长度
	int nfds;				//使用的数组长度
	//int realloc_copy;				//这里不用
	struct pollfd *event_set;		//pollfd数组
	//struct pollfd *event_set_copy;	//这里不用
};

static void *poll_init(struct event_base *);
static int poll_add(struct event_base *, int, short old, short events, void *idx);
static int poll_del(struct event_base *, int, short old, short events, void *idx);
static int poll_dispatch(struct event_base *, struct timeval *);
static void poll_dealloc(struct event_base *);

const struct eventop pollops = {
	"poll",
	poll_init,
	poll_add,
	poll_del,
	poll_dispatch,
	poll_dealloc,
	0, /* doesn't need_reinit */
	EV_FEATURE_FDS,
	sizeof(struct pollidx),
};

static void *poll_init(struct event_base *base)
{
	struct pollop *pollop;

	if (!(pollop = mm_calloc(1, sizeof(struct pollop))))
		return (NULL);

	return (pollop);
}

static int poll_dispatch(struct event_base *base, struct timeval *tv)
{
	int res, i, j, nfds;
	long msec = -1;
	struct pollop *pop = base->evbase;
	struct pollfd *event_set;

	nfds = pop->nfds;

	event_set = pop->event_set;

	if (tv != NULL) {
		//msec = evutil_tv_to_msec_(tv);
		msec = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
		if (msec < 0 || msec > INT_MAX)
			msec = INT_MAX;
	}

	res = poll(event_set, nfds, msec);

	if (res == -1) {
		return (0);
	}

	if (res == 0 || nfds == 0)
		return (0);

	//i = evutil_weakrand_range_(&base->weakrand_seed, nfds);
	i = -1;
	for (j = 0; j < nfds; j++) {
		int what;
		if (++i == nfds)
			i = 0;
		what = event_set[i].revents;
		if (!what)
			continue;

		res = 0;
		if (what & (POLLHUP|POLLERR|POLLNVAL))
			what |= POLLIN|POLLOUT;
		if (what & POLLIN)
			res |= EV_READ;
		if (what & POLLOUT)
			res |= EV_WRITE;
		if (res == 0)
			continue;

		evmap_io_active_(base, event_set[i].fd, res);
	}

	return (0);
}

static int poll_add(struct event_base *base, int fd, short old, short events, void *idx_)
{
	struct pollop *pop = base->evbase;
	struct pollfd *pfd = NULL;
	struct pollidx *idx = idx_;
	int i;

	if (!(events & (EV_READ|EV_WRITE)))
		return (0);

	if (pop->nfds + 1 >= pop->event_count) {
		struct pollfd *tmp_event_set;
		int tmp_event_count;

		if (pop->event_count < 32)
			tmp_event_count = 32;
		else
			tmp_event_count = pop->event_count * 2;

		tmp_event_set = mm_realloc(pop->event_set,
				 tmp_event_count * sizeof(struct pollfd));
		if (tmp_event_set == NULL) {
			return (-1);
		}
		pop->event_set = tmp_event_set;

		pop->event_count = tmp_event_count;
		//pop->realloc_copy = 1;
	}

	i = idx->idxplus1 - 1;

	if (i >= 0) {
		pfd = &pop->event_set[i];
	} else {
		i = pop->nfds++;
		pfd = &pop->event_set[i];
		pfd->events = 0;
		pfd->fd = fd;
		idx->idxplus1 = i + 1;
	}

	pfd->revents = 0;
	if (events & EV_WRITE)
		pfd->events |= POLLOUT;
	if (events & EV_READ)
		pfd->events |= POLLIN;

	return (0);
}

static int poll_del(struct event_base *base, int fd, short old, short events, void *idx_)
{
	struct pollop *pop = base->evbase;
	struct pollfd *pfd = NULL;
	struct pollidx *idx = idx_;
	int i;

	if (!(events & (EV_READ|EV_WRITE)))
		return (0);

	i = idx->idxplus1 - 1;
	if (i < 0)
		return (-1);

	pfd = &pop->event_set[i];
	if (events & EV_READ)
		pfd->events &= ~POLLIN;
	if (events & EV_WRITE)
		pfd->events &= ~POLLOUT;

	if (pfd->events)
		return (0);

	idx->idxplus1 = 0;

	--pop->nfds;
	if (i != pop->nfds) {
		memcpy(&pop->event_set[i], &pop->event_set[pop->nfds],
		       sizeof(struct pollfd));
		idx = evmap_io_get_fdinfo_(&base->io, pop->event_set[i].fd);
		idx->idxplus1 = i + 1;
	}
	return (0);
}

static void poll_dealloc(struct event_base *base)
{
	struct pollop *pop = base->evbase;

	if (pop->event_set)
		mm_free(pop->event_set);
	// if (pop->event_set_copy)
	// 	mm_free(pop->event_set_copy);

	memset(pop, 0, sizeof(struct pollop));
	mm_free(pop);
}

#endif /* EVENT__HAVE_POLL */
