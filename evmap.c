
#include <string.h>
#include "event-internal.h"
#include "evmap-internal.h"
#include "event_struct.h"
#include "mm-internal.h"
#include "event.h"

struct evmap_io {
	struct event_dlist events;		
	ev_uint16_t nread;
	ev_uint16_t nwrite;
	ev_uint16_t nclose;
};

struct event_map_entry {
	struct {
		struct event_map_entry *hte_next;
	} map_node;

	evutil_socket_t fd;
	union { 
		struct evmap_io evmap_io;
	} ent;
};

static unsigned hashsocket(struct event_map_entry *e)
{
	unsigned h = (unsigned) e->fd;
	h += (h >> 2) | (h << 30);
	return h;
}

static int eqsocket(struct event_map_entry *e1, struct event_map_entry *e2)
{
	return e1->fd == e2->fd;
}

//#define HT_PROTOTYPE(name, type, field, hashfn, eqfn)                   
int event_io_map_HT_GROW(struct event_io_map *ht, unsigned min_capacity);
void event_io_map_HT_CLEAR(struct event_io_map *ht);
int event_io_map_HT_REP_IS_BAD_(const struct event_io_map *ht);
static void event_io_map_HT_INIT(struct event_io_map *head) {
	head->hth_table_length = 0;
	head->hth_table = NULL;
	head->hth_n_entries = 0;
	head->hth_load_limit = 0;
	head->hth_prime_idx = -1;
}                                                                     
static struct event_map_entry **event_io_map_HT_FIND_P_(struct event_io_map *head, struct event_map_entry *elm)
{                                                          
	struct event_map_entry **p;
	if (!head->hth_table)
		return NULL;
	p = &((head)->hth_table[hashsocket(elm) % head->hth_table_length]);
	while (*p) {
		if (eqsocket(*p, elm))
			return p;
		p = &(*p)->map_node.hte_next;
	}
	return p;                                                           
}                                                                      
static struct event_map_entry *event_io_map_HT_FIND(const struct event_io_map *head, struct event_map_entry *elm)
{                                                                    
	struct event_map_entry **p;                                                    
	struct event_io_map *h = (struct event_io_map *) head;
	//HT_SET_HASH_(elm, map_node, hashsocket);
	p = event_io_map_HT_FIND_P_(h, elm);
	return p ? *p : NULL;                                               
}        
static void event_io_map_HT_INSERT(struct event_io_map *head, struct event_map_entry *elm)
{                                                      
	struct event_map_entry **p;
	if (!head->hth_table || head->hth_n_entries >= head->hth_load_limit)
		event_io_map_HT_GROW(head, head->hth_n_entries + 1);
	++head->hth_n_entries;                                              
	//HT_SET_HASH_(elm, map_node, hashsocket);
	p = &((head)->hth_table[hashsocket(elm) % head->hth_table_length]);
	elm->map_node.hte_next = *p;
	*p = elm;                                                           
}                                                                                                                              
static struct event_map_entry *event_io_map_HT_REPLACE(struct event_io_map *head, struct event_map_entry *elm)
{                                                      
	struct event_map_entry **p, *r;
	if (!head->hth_table || head->hth_n_entries >= head->hth_load_limit)
		event_io_map_HT_GROW(head, head->hth_n_entries + 1);
	//HT_SET_HASH_(elm, field, hashsocket);
	p = event_io_map_HT_FIND_P_(head, elm);
	r = *p;
	*p = elm;
	if (r && (r != elm)) {
		elm->map_node.hte_next = r->map_node.hte_next;
		r->map_node.hte_next = NULL;
		return r;
	}
	else {
		++head->hth_n_entries;
		return NULL;
	}
}                                                                              
static struct event_map_entry *event_io_map_HT_REMOVE(struct event_io_map *head, struct event_map_entry *elm)
{                                               
	struct event_map_entry **p, *r;
	//HT_SET_HASH_(elm, field, hashsocket);
	p = event_io_map_HT_FIND_P_(head, elm);
	if (!p || !*p)
		return NULL;
	r = *p;
	*p = r->map_node.hte_next;
	r->map_node.hte_next = NULL;
	--head->hth_n_entries;
	return r;
}                                                                                                               
static void event_io_map_HT_FOREACH_FN(struct event_io_map *head, int(*fn)(struct event_map_entry *, void *), void *data)
{
	unsigned idx;
	struct event_map_entry **p, **nextp, *next;
	if (!head->hth_table)
		return;
	for (idx = 0; idx < head->hth_table_length; ++idx) {
		p = &head->hth_table[idx];
		while (*p) {
			nextp = &(*p)->map_node.hte_next;
			next = *nextp;
			if (fn(*p, data)) {
				--head->hth_n_entries;
				*p = next;
			}
			else {
				p = nextp;
			}
		}
	}
}     
static struct event_map_entry **event_io_map_HT_START(struct event_io_map *head)
{
	unsigned b = 0;
	while (b < head->hth_table_length) {
		if (head->hth_table[b])
			return &head->hth_table[b];
		++b;
	}
	return NULL;
}                                                                   
static struct event_map_entry **event_io_map_HT_NEXT(struct event_io_map *head, struct event_map_entry **elm)
{
	if ((*elm)->map_node.hte_next) {
		return &(*elm)->map_node.hte_next;
	}
	else {
		unsigned b = (hashsocket(*elm) % head->hth_table_length) + 1;
		while (b < head->hth_table_length) {
			if (head->hth_table[b])
				return &head->hth_table[b];
			++b;
		}
		return NULL;
	}
}
static struct event_map_entry **event_io_map_HT_NEXT_RMV(struct event_io_map *head, struct event_map_entry **elm)
{
	unsigned h = hashsocket(*elm);
	*elm = (*elm)->map_node.hte_next;
	--head->hth_n_entries;
	if (*elm) {
		return elm;
	}
	else {
		unsigned b = (h % head->hth_table_length) + 1;
		while (b < head->hth_table_length) {
			if (head->hth_table[b])
				return &head->hth_table[b];
			++b;
		}
		return NULL;
	}
}

//HT_GENERATE(event_io_map, event_map_entry, map_node, hashsocket, eqsocket,0.5, mm_malloc, mm_realloc, mm_free)                              
static unsigned event_io_map_PRIMES[] = { 
	53, 97, 193, 389, 
	769, 1543, 3079, 6151, 
	12289, 24593, 49157, 98317, 
	196613, 393241, 786433, 1572869, 
	3145739, 6291469, 12582917, 25165843, 
	50331653, 100663319, 201326611, 402653189, 
	805306457, 1610612741                                               
};                                                                    
static unsigned event_io_map_N_PRIMES = (unsigned)(sizeof(event_io_map_PRIMES) / sizeof(event_io_map_PRIMES[0]));                                                               
int event_io_map_HT_GROW(struct event_io_map *head, unsigned size)                      
{                                                          
	unsigned new_len, new_load_limit;
	int prime_idx;
	struct event_map_entry **new_table;
	if (head->hth_prime_idx == (int)event_io_map_N_PRIMES - 1)
		return 0;
	if (head->hth_load_limit > size)
		return 0;
	prime_idx = head->hth_prime_idx;
	do {
		new_len = event_io_map_PRIMES[++prime_idx];
		new_load_limit = (unsigned)(0.5*new_len);
	} while (new_load_limit <= size && prime_idx < (int)event_io_map_N_PRIMES);
	if ((new_table = mm_malloc(new_len*sizeof(struct event_map_entry*)))) {
		unsigned b;
		memset(new_table, 0, new_len*sizeof(struct event_map_entry*));
		for (b = 0; b < head->hth_table_length; ++b) {
			struct event_map_entry *elm, *next;
			unsigned b2;
			elm = head->hth_table[b];
			while (elm) {
				next = elm->map_node.hte_next;
				b2 = hashsocket(elm) % new_len;
				elm->map_node.hte_next = new_table[b2];
				new_table[b2] = elm;
				elm = next;
			}
		}
		if (head->hth_table)
			mm_free(head->hth_table);
		head->hth_table = new_table;
	}
	else {
		unsigned b, b2;
		new_table = mm_realloc(head->hth_table, new_len*sizeof(struct event_map_entry*));
		if (!new_table) 
			return -1;
		memset(new_table + head->hth_table_length, 0,(new_len - head->hth_table_length)*sizeof(struct event_map_entry*));
		for (b = 0; b < head->hth_table_length; ++b) {
			struct event_map_entry *e, **pE;
			for (pE = &new_table[b], e = *pE; e != NULL; e = *pE) {
				b2 = hashsocket(e) % new_len;
				if (b2 == b) {
					pE = &e->map_node.hte_next;
				}
				else {
					*pE = e->map_node.hte_next;
					e->map_node.hte_next = new_table[b2];
					new_table[b2] = e;
				}
			}
		}
		head->hth_table = new_table;
	}
	head->hth_table_length = new_len;
	head->hth_prime_idx = prime_idx;
	head->hth_load_limit = new_load_limit;
	return 0;
}                                                                                                               
void event_io_map_HT_CLEAR(struct event_io_map *head)                                    
{
	if (head->hth_table)
		mm_free(head->hth_table);
	event_io_map_HT_INIT(head);
}                                        
int event_io_map_HT_REP_IS_BAD_(const struct event_io_map *head)			
{                                                    
	unsigned n, i;
	struct event_map_entry *elm;
	if (!head->hth_table_length) {
		if (!head->hth_table && !head->hth_n_entries &&
			!head->hth_load_limit && head->hth_prime_idx == -1)
			return 0;
		else
			return 1;
	}
	if (!head->hth_table || head->hth_prime_idx < 0 ||
		!head->hth_load_limit)
		return 2;
	if (head->hth_n_entries > head->hth_load_limit)
		return 3;
	if (head->hth_table_length != event_io_map_PRIMES[head->hth_prime_idx])
		return 4;
	if (head->hth_load_limit != (unsigned)(0.5*head->hth_table_length))
		return 5;
	for (n = i = 0; i < head->hth_table_length; ++i) {
		for (elm = head->hth_table[i]; elm; elm = elm->map_node.hte_next) {
			if (hashsocket(elm) != hashsocket(elm))
				return 1000 + i;
			if ((hashsocket(elm) % head->hth_table_length) != i)
				return 10000 + i;
			++n;
		}
	}
	if (n != head->hth_n_entries)
		return 6;
	return 0;
}

void evmap_io_initmap_(struct event_io_map *ctx)
{
	//HT_INIT(event_io_map, ctx);
	event_io_map_HT_INIT(ctx);
}

void evmap_io_clear_(struct event_io_map *ctx)
{
	struct event_map_entry **ent, **next, *this;
	for (ent = event_io_map_HT_START(ctx); ent; ent = next) {
		this = *ent;
		next = event_io_map_HT_NEXT_RMV(ctx, ent);
		mm_free(this);
	}
	event_io_map_HT_CLEAR(ctx); 
}

static void evmap_io_init(struct evmap_io *entry)
{
	//LIST_INIT(&entry->events);
	(&entry->events)->lh_first = NULL;
	entry->nread = 0;
	entry->nwrite = 0;
	entry->nclose = 0;
}

int evmap_io_add_(struct event_base *base, evutil_socket_t fd, struct event *ev)
{
	const struct eventop *evsel = base->evsel;
	struct event_io_map *io = &base->io;
	struct evmap_io *ctx = NULL;
	int nread, nwrite, nclose, retval = 0;
	short res = 0, old = 0;

	if (fd < 0)
		return 0;

	//GET_IO_SLOT_AND_CTOR(ctx, io, fd, evmap_io, evmap_io_init,evsel->fdinfo_len);
	{
		struct event_map_entry key_, *ent_;
		key_.fd = fd;
		struct event_io_map *ptr_head_ = io;
		struct event_map_entry **ptr;
		if (!ptr_head_->hth_table || ptr_head_->hth_n_entries >= ptr_head_->hth_load_limit)
			event_io_map_HT_GROW(ptr_head_, ptr_head_->hth_n_entries + 1);
		//HT_SET_HASH_((&key_), map_node, hashsocket);
		ptr = event_io_map_HT_FIND_P_(ptr_head_, (&key_));
		if (*ptr) {
			ent_ = *ptr;
		}
		else {
			ent_ = mm_calloc(1, sizeof(struct event_map_entry) + evsel->fdinfo_len);
			if (ent_ == NULL)
				return (-1);
			ent_->fd = fd;
			(evmap_io_init)(&ent_->ent.evmap_io);
			//HT_FOI_INSERT_(map_node, io, &key_, ent_, ptr);
			{
				ent_->map_node.hte_next = NULL;
				*ptr = ent_;
				++((io)->hth_n_entries);
			}
		}
		(ctx) = &ent_->ent.evmap_io;
	}
	
	nread = ctx->nread;
	nwrite = ctx->nwrite;
	nclose = ctx->nclose;

	if (nread)
		old |= EV_READ;
	if (nwrite)
		old |= EV_WRITE;
	if (nclose)
		old |= EV_CLOSED;

	if (ev->ev_events & EV_READ) {
		if (++nread == 1)
			res |= EV_READ;
	}
	if (ev->ev_events & EV_WRITE) {
		if (++nwrite == 1)
			res |= EV_WRITE;
	}
	if (ev->ev_events & EV_CLOSED) {
		if (++nclose == 1)
			res |= EV_CLOSED;
	}

	if (res) {
		void *extra = ((char*)ctx) + sizeof(struct evmap_io);
		if (evsel->add(base, ev->ev_fd,old, (ev->ev_events & EV_ET) | res, extra) == -1)
			return (-1);
		retval = 1;
	}

	ctx->nread = (ev_uint16_t)nread;
	ctx->nwrite = (ev_uint16_t)nwrite;
	ctx->nclose = (ev_uint16_t)nclose;
	//LIST_INSERT_HEAD(&ctx->events, ev, ev_io_next);
	if ((ev->ev_.ev_io.ev_io_next.le_next = (&ctx->events)->lh_first) != NULL)
		(&ctx->events)->lh_first->ev_.ev_io.ev_io_next.le_prev = &ev->ev_.ev_io.ev_io_next.le_next;
	(&ctx->events)->lh_first = (ev);
	ev->ev_.ev_io.ev_io_next.le_prev = &(&ctx->events)->lh_first;

	return (retval);
}
 
int evmap_io_del_(struct event_base *base, evutil_socket_t fd, struct event *ev)
{
	const struct eventop *evsel = base->evsel;
	struct event_io_map *io = &base->io;
	struct evmap_io *ctx;
	int nread, nwrite, nclose, retval = 0;
	short res = 0, old = 0;

	if (fd < 0)
		return 0;

	//GET_IO_SLOT(ctx, io, fd, evmap_io);
	{
		struct event_map_entry key_, *ent_;
		key_.fd = fd;
		ent_ = event_io_map_HT_FIND(io, &key_);
		ctx = ent_ ? &ent_->ent.evmap_io : NULL;
	}

	nread = ctx->nread;
	nwrite = ctx->nwrite;
	nclose = ctx->nclose;

	if (nread)
		old |= EV_READ;
	if (nwrite)
		old |= EV_WRITE;
	if (nclose)
		old |= EV_CLOSED;

	if (ev->ev_events & EV_READ) {
		if (--nread == 0)
			res |= EV_READ;
		//EVUTIL_ASSERT(nread >= 0);
	}
	if (ev->ev_events & EV_WRITE) {
		if (--nwrite == 0)
			res |= EV_WRITE;
		//EVUTIL_ASSERT(nwrite >= 0);
	}
	if (ev->ev_events & EV_CLOSED) {
		if (--nclose == 0)
			res |= EV_CLOSED;
		//EVUTIL_ASSERT(nclose >= 0);
	}

	if (res) {
		void *extra = ((char*)ctx) + sizeof(struct evmap_io);
		if (evsel->del(base, ev->ev_fd,old, (ev->ev_events & EV_ET) | res, extra) == -1) {
			retval = -1;
		} else {
			retval = 1;
		}
	}

	ctx->nread = nread;
	ctx->nwrite = nwrite;
	ctx->nclose = nclose;
	//LIST_REMOVE(ev, ev_io_next);
	if (ev->ev_.ev_io.ev_io_next.le_next != NULL)
		ev->ev_.ev_io.ev_io_next.le_next->ev_.ev_io.ev_io_next.le_prev =ev->ev_.ev_io.ev_io_next.le_prev;
	*ev->ev_.ev_io.ev_io_next.le_prev = ev->ev_.ev_io.ev_io_next.le_next;

	return (retval);
}

void evmap_io_active_(struct event_base *base, evutil_socket_t fd, short events)
{
	struct event_io_map *io = &base->io;
	struct evmap_io *ctx;
	struct event *ev;

	//GET_IO_SLOT(ctx, io, fd, evmap_io);
	{
		struct event_map_entry key_, *ent_;
		key_.fd = fd;
		ent_ = event_io_map_HT_FIND(io, &key_);
		ctx = ent_ ? &ent_->ent.evmap_io : NULL;
	}

	if (NULL == ctx)
		return;
	for (ev = (&ctx->events)->lh_first; ev != NULL; ev = (ev)->ev_.ev_io.ev_io_next.le_next) {
		if (ev->ev_events & events)
			event_active_nolock_(ev, ev->ev_events & events, 1);
	}
}

void *evmap_io_get_fdinfo_(struct event_io_map *map, evutil_socket_t fd)
{
	struct evmap_io *ctx;
	//GET_IO_SLOT(ctx, map, fd, evmap_io);
	{
		struct event_map_entry key_, *ent_;
		key_.fd = fd;
		ent_ = event_io_map_HT_FIND(map, &key_);
		ctx = ent_ ? &ent_->ent.evmap_io : NULL;
	}

	if (ctx)
		return ((char*)ctx) + sizeof(struct evmap_io);
	else
		return NULL;
}

typedef int(*evmap_io_foreach_fd_cb)(struct event_base *, evutil_socket_t, struct evmap_io *, void *);
static int evmap_io_foreach_fd(struct event_base *base,evmap_io_foreach_fd_cb fn,void *arg)
{
	evutil_socket_t fd;
	struct event_io_map *iomap = &base->io;
	int r = 0;
	struct event_map_entry **mapent;

	for (mapent = event_io_map_HT_START(iomap); mapent != NULL; mapent = event_io_map_HT_NEXT(iomap, mapent)) {
		struct evmap_io *ctx = &(*mapent)->ent.evmap_io;
		fd = (*mapent)->fd;
		if ((r = fn(base, fd, ctx, arg)))
			break;
	}
	return r;
}

static int delete_all_in_dlist(struct event_dlist *dlist)
{
	struct event *ev;
	while ((ev = (dlist)->lh_first))
		event_del(ev);
	return 0;
}

static int evmap_io_delete_all_iter_fn(struct event_base *base, evutil_socket_t fd,struct evmap_io *io_info, void *arg)
{
	return delete_all_in_dlist(&io_info->events);
}

void evmap_delete_all_(struct event_base *base)
{
	evmap_io_foreach_fd(base, evmap_io_delete_all_iter_fn, NULL);
}
