
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "event-config.h"
#include "util.h"
#include "event_struct.h"
#include "event.h"
#include "event-internal.h"
#include "evmap-internal.h"
#include "mm-internal.h"
#include "time-internal.h"

#ifdef EVENT__HAVE_EVENT_PORTS
extern const struct eventop evportops;
#endif
#ifdef EVENT__HAVE_SELECT
extern const struct eventop selectops;
#endif
#ifdef EVENT__HAVE_POLL
extern const struct eventop pollops;
#endif
#ifdef EVENT__HAVE_EPOLL
extern const struct eventop epollops;
#endif
#ifdef EVENT__HAVE_WORKING_KQUEUE
extern const struct eventop kqops;
#endif
#ifdef EVENT__HAVE_DEVPOLL
extern const struct eventop devpollops;
#endif
#ifdef _WIN32
extern const struct eventop win32ops;
#endif

static const struct eventop *eventops[] = {
#ifdef EVENT__HAVE_EVENT_PORTS
	&evportops,
#endif
#ifdef EVENT__HAVE_WORKING_KQUEUE
	&kqops,
#endif
#ifdef EVENT__HAVE_EPOLL
	&epollops,
#endif
#ifdef EVENT__HAVE_DEVPOLL
	&devpollops,
#endif
#ifdef EVENT__HAVE_POLL
	&pollops,
#endif
#ifdef EVENT__HAVE_SELECT
	&selectops,
#endif
#ifdef _WIN32
	&win32ops,
#endif
	NULL
};
static void *event_self_cbarg_ptr_ = NULL;

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
static void	event_queue_insert_active(struct event_base *, struct event_callback *);
static void	event_queue_remove_active(struct event_base *, struct event_callback *);
static void	event_queue_insert_inserted(struct event_base *, struct event *);
static void	event_queue_remove_inserted(struct event_base *, struct event *);
static void	event_queue_insert_timeout(struct event_base *, struct event *);
static void	event_queue_remove_timeout(struct event_base *, struct event *);
static void	event_persist_closure(struct event_base *, struct event *ev);
static int	event_process_active(struct event_base *);
static int  event_del_(struct event *ev, int blocking);
static void	timeout_process(struct event_base *);

static struct event *event_callback_to_event(struct event_callback *evcb)
{
	return ((struct event *)(((char*)(evcb)) - offsetof(struct event, ev_evcallback)));
}

static struct event_callback * event_to_event_callback(struct event *ev)
{
	return &ev->ev_evcallback;
}

static void clear_time_cache(struct event_base *base)
{
	base->tv_cache.tv_sec = 0;
}

#define CLOCK_SYNC_INTERVAL 5
static int gettime(struct event_base *base, struct timeval *tp)
{
	if (base->tv_cache.tv_sec) {
		*tp = base->tv_cache;
		return (0);
	}

	if (evutil_gettime_monotonic_(&base->monotonic_timer, tp) == -1) {
		return -1;
	}

	if (base->last_updated_clock_diff + CLOCK_SYNC_INTERVAL < tp->tv_sec) {
		struct timeval tv;
		evutil_gettimeofday(&tv, NULL);
		evutil_timersub(&tv, tp, &base->tv_clock_diff);
		base->last_updated_clock_diff = tp->tv_sec;
	}

	return 0;
}

static void update_time_cache(struct event_base *base)
{
	base->tv_cache.tv_sec = 0;
	if (!(base->flags & EVENT_BASE_FLAG_NO_CACHE_TIME))
		gettime(base, &base->tv_cache);
}

static int event_config_is_avoided_method(const struct event_config *cfg,const char *method)
{
	struct event_config_entry *entry;

	for (entry = (&cfg->entries)->tqh_first; entry != NULL; entry = entry->next.tqe_next) {
		if (entry->avoid_method != NULL &&
			strcmp(entry->avoid_method, method) == 0)
			return (1);
	}

	return (0);
}

struct event_base *event_base_new(void)
{
	struct event_base *base = NULL;
	struct event_config *cfg = event_config_new();
	if (cfg) {
		base = event_base_new_with_config(cfg);
		event_config_free(cfg);
	}
	return base;
}

struct event_config *event_config_new(void)
{
	struct event_config *cfg = mm_calloc(1, sizeof(*cfg));

	if (cfg == NULL)
		return (NULL);

	//TAILQ_INIT(&cfg->entries);
	(&cfg->entries)->tqh_first = NULL;
	(&cfg->entries)->tqh_last = &(&cfg->entries)->tqh_first;

	return (cfg);
}

struct event_base *event_base_new_with_config(const struct event_config *cfg)
{
	int i;
	struct event_base *base;

	if ((base = mm_calloc(1, sizeof(struct event_base))) == NULL) {
		return NULL;
	}

	int precise_time = cfg && (cfg->flags & EVENT_BASE_FLAG_PRECISE_TIMER);
	int flags;
	struct timeval tmp;
	flags = precise_time ? EV_MONOT_PRECISE : 0;
	evutil_configure_monotonic_time_(&base->monotonic_timer, flags);
	gettime(base, &tmp);

	min_heap_ctor_(&base->timeheap);

	evmap_io_initmap_(&base->io);

	base->evbase = NULL;

	for (i = 0; eventops[i] && !base->evbase; i++) {
		if (cfg != NULL) {
			if (event_config_is_avoided_method(cfg,eventops[i]->name))
				continue;
		}

		base->evsel = eventops[i];
		base->evbase = base->evsel->init(base);
	}

	if (base->evbase == NULL) {
		base->evsel = NULL;
		event_base_free(base);
		printf("evbase == NULL");
		return NULL;
	}

	if (event_base_priority_init(base, 1) < 0) {
		event_base_free(base);
		return NULL;
	}

	return (base);
}

static void event_config_entry_free(struct event_config_entry *entry)
{
	if (entry->avoid_method != NULL)
		mm_free((char *)entry->avoid_method);
	mm_free(entry);
}

void event_config_free(struct event_config *cfg)
{
	struct event_config_entry *entry;

	while ((entry = (&cfg->entries)->tqh_first) != NULL) {
		if ((entry->next.tqe_next) != NULL)				
			entry->next.tqe_next->next.tqe_prev = entry->next.tqe_prev;				
		else								
			(&cfg->entries)->tqh_last = entry->next.tqe_prev;		
		*entry->next.tqe_prev = entry->next.tqe_next;			
		event_config_entry_free(entry);
	}
	mm_free(cfg);
}

int event_config_set_flag(struct event_config *cfg, int flag)
{
	if (!cfg)
		return -1;
	cfg->flags |= flag;
	return 0;
}

int event_config_avoid_method(struct event_config *cfg, const char *method)
{
	struct event_config_entry *entry = mm_malloc(sizeof(*entry));
	if (entry == NULL)
		return (-1);

	if ((entry->avoid_method = mm_strdup(method)) == NULL) {
		mm_free(entry);
		return (-1);
	}

	//TAILQ_INSERT_TAIL(&cfg->entries, entry, next);
	entry->next.tqe_next = NULL;
	entry->next.tqe_prev = (&cfg->entries)->tqh_last;
	*(&cfg->entries)->tqh_last = entry;
	(&cfg->entries)->tqh_last = &entry->next.tqe_next;

	return (0);
}

static void event_base_free_(struct event_base *base, int run_finalizers)
{
	if (base == NULL) {
		return;
	}

	evmap_delete_all_(base);

	if (base->evsel != NULL && base->evsel->dealloc != NULL)
		base->evsel->dealloc(base);

	mm_free(base->activequeues);
	evmap_io_clear_(&base->io);

	mm_free(base);
}

void event_base_free(struct event_base *base)
{
	event_base_free_(base, 1);
}

int event_base_priority_init(struct event_base *base, int npriorities)
{
	int i, r;
	r = -1;

	if ((base)->event_count_active || npriorities < 1 || npriorities >= EVENT_MAX_PRIORITIES)
		goto err;

	if (npriorities == base->nactivequeues)
		goto ok;

	if (base->nactivequeues) {
		mm_free(base->activequeues);
		base->nactivequeues = 0;
	}

	base->activequeues = (struct evcallback_list *)mm_calloc(npriorities, sizeof(struct evcallback_list));
	if (base->activequeues == NULL) {
		goto err;
	}
	base->nactivequeues = npriorities;

	for (i = 0; i < base->nactivequeues; ++i) {
		(&base->activequeues[i])->tqh_first = NULL;
		(&base->activequeues[i])->tqh_last = &(&base->activequeues[i])->tqh_first;
	}
ok:
	r = 0;
err:
	return (r);
}

int event_assign(struct event *ev, struct event_base *base, evutil_socket_t fd, short events, void(*callback)(evutil_socket_t, short, void *), void *arg)
{
	if (arg == &event_self_cbarg_ptr_)
		arg = ev;

	ev->ev_base = base;

	ev->ev_evcallback.evcb_cb_union.evcb_callback = callback;
	ev->ev_evcallback.evcb_arg = arg;
	ev->ev_fd = fd;
	ev->ev_events = events;
	ev->ev_res = 0;
	ev->ev_evcallback.evcb_flags = EVLIST_INIT;

	if (events & EV_PERSIST) {
		evutil_timerclear(&ev->ev_.ev_io.ev_timeout);
		ev->ev_evcallback.evcb_closure = EV_CLOSURE_EVENT_PERSIST;
	}
	else {
		ev->ev_evcallback.evcb_closure = EV_CLOSURE_EVENT;
	}

	if (base != NULL) {
		ev->ev_evcallback.evcb_pri = base->nactivequeues / 2;
	}

	return 0;
}

struct event *event_new(struct event_base *base, evutil_socket_t fd, short events, void(*cb)(evutil_socket_t, short, void *), void *arg)
{
	struct event *ev;
	ev = mm_malloc(sizeof(struct event));
	if (ev == NULL)
		return (NULL);
	if (event_assign(ev, base, fd, events, cb, arg) < 0) {
		mm_free(ev);
		return (NULL);
	}

	return (ev);
}

int event_add(struct event *ev, const struct timeval *tv)
{
	int res;

	res = event_add_nolock_(ev, tv, 0);
	
	return (res);
}

int event_add_nolock_(struct event *ev, const struct timeval *tv,int tv_is_absolute)
{
	struct event_base *base = ev->ev_base;
	int res = 0;
	int notify = 0;

	if (tv != NULL && !(ev->ev_evcallback.evcb_flags & EVLIST_TIMEOUT)) {
		if (min_heap_reserve_(&base->timeheap,1 + min_heap_size_(&base->timeheap)) == -1)
			return (-1); 
	}

	if ((ev->ev_events & (EV_READ | EV_WRITE | EV_CLOSED )) && !(ev->ev_evcallback.evcb_flags & (EVLIST_INSERTED | EVLIST_ACTIVE ))) {
		if (ev->ev_events & (EV_READ | EV_WRITE | EV_CLOSED))
			res = evmap_io_add_(base, ev->ev_fd, ev);
		if (res != -1)
			event_queue_insert_inserted(base, ev);
		if (res == 1) {
			res = 0;
		}
	}

	if (res != -1 && tv != NULL) {
		struct timeval now;

		if (ev->ev_evcallback.evcb_closure == EV_CLOSURE_EVENT_PERSIST && !tv_is_absolute)
			ev->ev_.ev_io.ev_timeout = *tv;

		if ((ev->ev_evcallback.evcb_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)) {
			event_queue_remove_active(base, event_to_event_callback(ev));
		}

		gettime(base, &now);
		if (tv_is_absolute) {
			ev->ev_timeout = *tv;
		}
		else {
			evutil_timeradd(&now, tv, &ev->ev_timeout);
		}

		event_queue_insert_timeout(base, ev);
	}
	return (res);
}

int event_base_dispatch(struct event_base *event_base)
{
	return (event_base_loop(event_base, 0));
}

int event_base_loop(struct event_base *base, int flags)
{
	const struct eventop *evsel = base->evsel;
	int res, done, retval = 0;
	struct timeval tv;
	struct timeval *tv_p;
	done = 0;

	while (!done) {
		tv_p = &tv;

		evutil_timerclear(&tv);

		clear_time_cache(base);
		res = evsel->dispatch(base, &tv);

		if (res == -1) {
			retval = -1;
			goto done;
		}
		update_time_cache(base);
		timeout_process(base);
		if (base->event_count_active) {
			int n = event_process_active(base);
			if ((flags & EVLOOP_ONCE) && base->event_count_active == 0 && n != 0)
				done = 1;
		}
		else if (flags & EVLOOP_NONBLOCK)
			done = 1;
	}
done:
	return (retval);
}

void event_active_nolock_(struct event *ev, int res, short ncalls)
{
	struct event_base *base;

	base = ev->ev_base;

	switch ((ev->ev_evcallback.evcb_flags & (EVLIST_ACTIVE | EVLIST_ACTIVE_LATER))) {
	default:
	case EVLIST_ACTIVE | EVLIST_ACTIVE_LATER:
		break;
	case EVLIST_ACTIVE:
		ev->ev_res |= res;
		return;
	case EVLIST_ACTIVE_LATER:
		ev->ev_res |= res;
		break;
	case 0:
		ev->ev_res = res;
		break;
	}

	event_callback_activate_nolock_(base, event_to_event_callback(ev));
}

int event_callback_activate_nolock_(struct event_base *base,struct event_callback *evcb)
{
	int r = 1;

	event_queue_insert_active(base, evcb);

	return r;
}

int event_del(struct event *ev)
{
	return event_del_(ev, EVENT_DEL_AUTOBLOCK);
}

int event_del_nolock_(struct event *ev, int blocking)
{
	struct event_base *base;
	int res = 0;

	if (ev->ev_base == NULL)
		return (-1);

	base = ev->ev_base;

	if (ev->ev_evcallback.evcb_flags & EVLIST_TIMEOUT) {
		event_queue_remove_timeout(base, ev);
	}

	if (ev->ev_evcallback.evcb_flags & EVLIST_ACTIVE)
		event_queue_remove_active(base, event_to_event_callback(ev));

	if (ev->ev_evcallback.evcb_flags & EVLIST_INSERTED) {
		event_queue_remove_inserted(base, ev);
		if (ev->ev_events & (EV_READ | EV_WRITE | EV_CLOSED))
			res = evmap_io_del_(base, ev->ev_fd, ev);
		if (res == 1) {
			res = 0;
		}
	}
	return (res);
}

void event_free(struct event *ev)
{
	event_del(ev);
	mm_free(ev);
}

void *event_self_cbarg(void)
{
	return &event_self_cbarg_ptr_;
}

evutil_socket_t event_get_fd(const struct event *ev)
{
	return ev->ev_fd;
}


static void event_persist_closure(struct event_base *base, struct event *ev)
{
	void(*evcb_callback)(evutil_socket_t, short, void *);

	evutil_socket_t evcb_fd;
	short evcb_res;
	void *evcb_arg;

	if (ev->ev_.ev_io.ev_timeout.tv_sec || ev->ev_.ev_io.ev_timeout.tv_usec) {
		struct timeval run_at, relative_to, delay, now;
		ev_uint32_t usec_mask = 0;

		gettime(base, &now);
		delay = ev->ev_.ev_io.ev_timeout;
		if (ev->ev_res & EV_TIMEOUT) {
			relative_to = ev->ev_timeout;
		}
		else {
			relative_to = now;
		}
		
		evutil_timeradd(&relative_to, &delay, &run_at);
		if (evutil_timercmp(&run_at, &now, <)) {
			evutil_timeradd(&now, &delay, &run_at);
		}
		run_at.tv_usec |= usec_mask;
		event_add_nolock_(ev, &run_at, 1);
	}


	evcb_callback = ev->ev_evcallback.evcb_cb_union.evcb_callback;
	evcb_fd = ev->ev_fd;
	evcb_res = ev->ev_res;
	evcb_arg = ev->ev_evcallback.evcb_arg;

	(evcb_callback)(evcb_fd, evcb_res, evcb_arg);
}

static int event_process_active_single_queue(struct event_base *base, struct evcallback_list *activeq, int max_to_process, const struct timeval *endtime)
{
	struct event_callback *evcb;
	int count = 0;

	for (evcb = activeq->tqh_first; evcb; evcb = activeq->tqh_first) {
		struct event *ev = NULL;
		if (evcb->evcb_flags & EVLIST_INIT) {
			ev = event_callback_to_event(evcb);

			if (ev->ev_events & EV_PERSIST || ev->ev_evcallback.evcb_flags & EVLIST_FINALIZING)
				event_queue_remove_active(base, evcb);
			else
				event_del_nolock_(ev, EVENT_DEL_NOBLOCK);
		}
		else {
			event_queue_remove_active(base, evcb);
		}

		if (!(evcb->evcb_flags & EVLIST_INTERNAL))
			++count;
		base->current_event = evcb;

		switch (evcb->evcb_closure) {
		case EV_CLOSURE_EVENT_PERSIST:
			event_persist_closure(base, ev);
			break;
		case EV_CLOSURE_EVENT: {
								   void(*evcb_callback)(evutil_socket_t, short, void *);
								   short res;
								   evcb_callback = *ev->ev_evcallback.evcb_cb_union.evcb_callback;
								   res = ev->ev_res;
								   evcb_callback(ev->ev_fd, res, ev->ev_evcallback.evcb_arg);
		}
			break;
		case EV_CLOSURE_CB_FINALIZE: {
										 void(*evcb_cbfinalize)(struct event_callback *, void *) = evcb->evcb_cb_union.evcb_cbfinalize;
										 base->current_event = NULL;
										 evcb_cbfinalize(evcb, evcb->evcb_arg);
		}
			break;
		default:
			break;
		}
		base->current_event = NULL;

		if (count >= max_to_process)
			return count;
	}
	return count;
}

static int event_process_active(struct event_base *base)
{
	struct evcallback_list *activeq = NULL;
	int i, c = 0;

	for (i = 0; i < base->nactivequeues; ++i) {
		if ((&base->activequeues[i])->tqh_first != NULL) {
			activeq = &base->activequeues[i];
			c = event_process_active_single_queue(base, activeq, INT_MAX, NULL);
			if (c < 0) {
				goto done;
			}
			else if (c > 0)
				break;	
		}
	}

done:
	return c;
}

static void event_queue_insert_inserted(struct event_base *base, struct event *ev)
{
	ev->ev_evcallback.evcb_flags |= EVLIST_INSERTED;
}

static void event_queue_remove_inserted(struct event_base *base, struct event *ev)
{
	ev->ev_evcallback.evcb_flags &= ~EVLIST_INSERTED;
}

static void event_queue_insert_active(struct event_base *base, struct event_callback *evcb)
{
	if (evcb->evcb_flags & EVLIST_ACTIVE) {
		return;
	}

	evcb->evcb_flags |= EVLIST_ACTIVE;

	base->event_count_active++;
	base->event_count_active_max = MAX(base->event_count_active_max, base->event_count_active);
	//TAILQ_INSERT_TAIL(&base->activequeues[evcb->evcb_pri], evcb, evcb_active_next);
	{
		evcb->evcb_active_next.tqe_next = NULL;
		evcb->evcb_active_next.tqe_prev = (&base->activequeues[evcb->evcb_pri])->tqh_last;
		*(&base->activequeues[evcb->evcb_pri])->tqh_last = evcb;
		(&base->activequeues[evcb->evcb_pri])->tqh_last = &(evcb)->evcb_active_next.tqe_next;
	}
}

static void event_queue_remove_active(struct event_base *base, struct event_callback *evcb)
{
	evcb->evcb_flags &= ~EVLIST_ACTIVE;
	base->event_count_active--;

	//TAILQ_REMOVE(&base->activequeues[evcb->evcb_pri],evcb, evcb_active_next);
	{
		if (((evcb)->evcb_active_next.tqe_next) != NULL)
			(evcb)->evcb_active_next.tqe_next->evcb_active_next.tqe_prev =
			(evcb)->evcb_active_next.tqe_prev;
		else
			(&base->activequeues[evcb->evcb_pri])->tqh_last = (evcb)->evcb_active_next.tqe_prev;
		*(evcb)->evcb_active_next.tqe_prev = (evcb)->evcb_active_next.tqe_next;
	}
}

static int event_del_(struct event *ev, int blocking)
{
	int res;
	struct event_base *base = ev->ev_base;

	res = event_del_nolock_(ev, blocking);

	return (res);
}

const char *event_base_get_method(const struct event_base *base)
{
	return (base->evsel->name);
}

static void event_queue_insert_timeout(struct event_base *base, struct event *ev)
{
	ev->ev_evcallback.evcb_flags |= EVLIST_TIMEOUT;

	min_heap_push_(&base->timeheap, ev);
}

static void event_queue_remove_timeout(struct event_base *base, struct event *ev)
{
	ev->ev_evcallback.evcb_flags &= ~EVLIST_TIMEOUT;

	min_heap_erase_(&base->timeheap, ev);
}

static void timeout_process(struct event_base *base)
{
	struct timeval now;
	struct event *ev;

	if (min_heap_empty_(&base->timeheap)) {
		return;
	}

	gettime(base, &now);

	while ((ev = min_heap_top_(&base->timeheap))) {
		if (evutil_timercmp(&ev->ev_timeout, &now, > ))
			break;
		event_del_nolock_(ev, EVENT_DEL_NOBLOCK);
		event_active_nolock_(ev, EV_TIMEOUT, 1);
	}
}

int event_callback_cancel_nolock_(struct event_base *base,struct event_callback *evcb, int even_if_finalizing)
{
	if ((evcb->evcb_flags & EVLIST_FINALIZING) && !even_if_finalizing)
		return 0;

	if (evcb->evcb_flags & EVLIST_INIT)
		return event_del_nolock_(event_callback_to_event(evcb),even_if_finalizing ? EVENT_DEL_EVEN_IF_FINALIZING : EVENT_DEL_AUTOBLOCK);

	switch ((evcb->evcb_flags & (EVLIST_ACTIVE ))) {
	default:
		break;
	case EVLIST_ACTIVE:
		event_queue_remove_active(base, evcb);
		return 0;
	case 0:
		break;
	}

	return 0;
}

void event_callback_finalize_nolock_(struct event_base *base, unsigned flags, struct event_callback *evcb, void(*cb)(struct event_callback *, void *))
{
	struct event *ev = NULL;
	if (evcb->evcb_flags & EVLIST_INIT) {
		ev = event_callback_to_event(evcb);
		event_del_nolock_(ev, EVENT_DEL_NOBLOCK);
	}
	else {
		event_callback_cancel_nolock_(base, evcb, 0); /*XXX can this fail?*/
	}

	evcb->evcb_closure = EV_CLOSURE_CB_FINALIZE;
	evcb->evcb_cb_union.evcb_cbfinalize = cb;
	event_callback_activate_nolock_(base, evcb); /* XXX can this really fail?*/
	evcb->evcb_flags |= EVLIST_FINALIZING;
}


int event_callback_finalize_many_(struct event_base *base, int n_cbs, struct event_callback **evcbs, void(*cb)(struct event_callback *, void *))
{
	int n_pending = 0, i;

	for (i = 0; i < n_cbs; ++i) {
		struct event_callback *evcb = evcbs[i];
		if (evcb == base->current_event) {
			event_callback_finalize_nolock_(base, 0, evcb, cb);
			++n_pending;
		}
		else {
			event_callback_cancel_nolock_(base, evcb, 0);
		}
	}

	if (n_pending == 0) {
		event_callback_finalize_nolock_(base, 0, evcbs[0], cb);
	}

	return 0;
}