
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "event.h"
#include "buffer.h"
#include "bufferevent.h"
#include "bufferevent_struct.h"
#include "mm-internal.h"
#include "evbuffer-internal.h"
#include "bufferevent-internal.h"
#include "event-internal.h"

#define MAX_TO_COPY_IN_EXPAND 4096
#define EVBUFFER_MAX_READ	4096
#define CHAIN_SPACE_LEN(ch) (ch)->buffer_len - ((ch)->misalign + (ch)->off)

static struct evbuffer_chain *evbuffer_expand_singlechain(struct evbuffer *buf,size_t datlen);

static struct evbuffer_chain *evbuffer_chain_new(size_t size)
{
	struct evbuffer_chain *chain;
	size_t to_alloc;

	size += EVBUFFER_CHAIN_SIZE;

	to_alloc = MIN_BUFFER_SIZE;
	while (to_alloc < size) {
		to_alloc <<= 1;
	}

	if ((chain = mm_malloc(to_alloc)) == NULL)
		return (NULL);

	memset(chain, 0, EVBUFFER_CHAIN_SIZE);

	chain->buffer_len = to_alloc - EVBUFFER_CHAIN_SIZE;

	chain->buffer = (unsigned char *)((struct evbuffer_chain *)(chain)+1);

	return (chain);
}

static void evbuffer_chain_free(struct evbuffer_chain *chain)
{
	mm_free(chain);
}

static void evbuffer_free_all_chains(struct evbuffer_chain *chain)
{//释放chain和后面的所有chain
	struct evbuffer_chain *next;
	for (; chain; chain = next) {
		next = chain->next;
		evbuffer_chain_free(chain);
	}
}

static struct evbuffer_chain **evbuffer_free_trailing_empty_chains(struct evbuffer *buf)
{//从last_with_datap往后找空的chain释放，返回最后一个数据的next
	struct evbuffer_chain **ch = buf->last_with_datap;
	while ((*ch) && ((*ch)->off != 0 ))
		ch = &(*ch)->next;
	if (*ch) {
		evbuffer_free_all_chains(*ch);
		*ch = NULL;
	}
	return ch;
}

static void evbuffer_chain_insert(struct evbuffer *buf,struct evbuffer_chain *chain)
{//移除尾部空的chain并插入chain
	if (*buf->last_with_datap == NULL) {
		buf->first = buf->last = chain;
	} else {
		struct evbuffer_chain **chp;
		chp = evbuffer_free_trailing_empty_chains(buf);
		*chp = chain;
		if (chain->off)
			buf->last_with_datap = chp;
		buf->last = chain;
	}
	buf->total_len += chain->off;
}

static struct evbuffer_chain *evbuffer_chain_insert_new(struct evbuffer *buf, size_t datlen)
{
	struct evbuffer_chain *chain;
	if ((chain = evbuffer_chain_new(datlen)) == NULL)
		return NULL;
	evbuffer_chain_insert(buf, chain);
	return chain;
}

struct evbuffer *evbuffer_new(void)
{
	struct evbuffer *buffer;

	buffer = mm_calloc(1, sizeof(struct evbuffer));
	if (buffer == NULL)
		return (NULL);

	//LIST_INIT(&buffer->callbacks);
	(&buffer->callbacks)->lh_first = NULL;
	buffer->last_with_datap = &buffer->first;

	return (buffer);
}

static void evbuffer_run_callbacks(struct evbuffer *buffer, int running_deferred)
{
	struct evbuffer_cb_entry *cbent, *next;
	struct evbuffer_cb_info info;
	size_t new_size;

	if ((&buffer->callbacks)->lh_first == NULL) {
		buffer->n_add_for_cb = buffer->n_del_for_cb = 0;
		return;
	}
	if (buffer->n_add_for_cb == 0 && buffer->n_del_for_cb == 0)
		return;

	new_size = buffer->total_len;
	info.orig_size = new_size + buffer->n_del_for_cb - buffer->n_add_for_cb;
	info.n_added = buffer->n_add_for_cb;
	info.n_deleted = buffer->n_del_for_cb;

	for (cbent = (&buffer->callbacks)->lh_first;cbent != NULL;cbent = next) {
		next = cbent->next.le_next;
		cbent->cb.cb_func(buffer, &info, cbent->cbarg);
	}
}

void evbuffer_invoke_callbacks_(struct evbuffer *buffer)
{
	if (((&buffer->callbacks)->lh_first) == NULL) {
		buffer->n_add_for_cb = buffer->n_del_for_cb = 0;
		return;
	}

	evbuffer_run_callbacks(buffer, 0);
}

static void evbuffer_remove_all_callbacks(struct evbuffer *buffer)
{
	struct evbuffer_cb_entry *cbent;

	while ((cbent = (&buffer->callbacks)->lh_first)) {
		//LIST_REMOVE(cbent, next);
		if ((cbent)->next.le_next != NULL)
			(cbent)->next.le_next->next.le_prev =(cbent)->next.le_prev;
		*(cbent)->next.le_prev = (cbent)->next.le_next;
		mm_free(cbent);
	}
}

void evbuffer_free(struct evbuffer *buffer)
{
	evbuffer_decref_and_unlock_(buffer);
}

size_t evbuffer_get_length(const struct evbuffer *buffer)
{
	size_t result;

	result = (buffer->total_len);

	return result;
}

static void ZERO_CHAIN(struct evbuffer *dst)
{
	dst->first = NULL;
	dst->last = NULL;
	dst->last_with_datap = &(dst)->first;
	dst->total_len = 0;
}

int evbuffer_drain(struct evbuffer *buf, size_t len)
{
	struct evbuffer_chain *chain, *next;
	size_t remaining, old_len;
	int result = 0;

	old_len = buf->total_len;

	if (old_len == 0)
		goto done;

	if (len >= old_len) {
		len = old_len;
		for (chain = buf->first; chain != NULL; chain = next) {
			next = chain->next;
			evbuffer_chain_free(chain);
		}
		ZERO_CHAIN(buf);
	} else {
		buf->total_len -= len;
		remaining = len;
		for (chain = buf->first; remaining >= chain->off; chain = next) {
			next = chain->next;
			remaining -= chain->off;

			if (chain == *buf->last_with_datap) {
				buf->last_with_datap = &buf->first;
			}
			if (&chain->next == buf->last_with_datap)
				buf->last_with_datap = &buf->first;

			evbuffer_chain_free(chain);
		}

		buf->first = chain;
		chain->misalign += remaining;
		chain->off -= remaining;
	}

	buf->n_del_for_cb += len;
	evbuffer_invoke_callbacks_(buf);
done:
	return result;
}

int evbuffer_remove(struct evbuffer *buf, void *data_out, size_t datlen)
{
	ev_ssize_t n;
	n = evbuffer_copyout_from(buf, NULL, data_out, datlen);
	if (n > 0) {
		if (evbuffer_drain(buf, n)<0)
			n = -1;
	}
	return (int)n;
}

ev_ssize_t
evbuffer_copyout(struct evbuffer *buf, void *data_out, size_t datlen)
{
	return evbuffer_copyout_from(buf, NULL, data_out, datlen);
}

unsigned char *evbuffer_pullup(struct evbuffer *buf, ev_ssize_t size)
{
	struct evbuffer_chain *chain, *next, *tmp, *last_with_data;
	unsigned char *buffer, *result = NULL;
	ev_ssize_t remaining;
	int removed_last_with_data = 0;
	int removed_last_with_datap = 0;

	chain = buf->first;

	if (size < 0)
		size = buf->total_len;

	if (size == 0 || (size_t)size > buf->total_len)
		goto done;

	if (chain->off >= (size_t)size) {
		result = chain->buffer + chain->misalign;
		goto done;
	}

	remaining = size - chain->off;
	for (tmp=chain->next; tmp; tmp=tmp->next) {
		if (tmp->off >= (size_t)remaining)
			break;
		remaining -= tmp->off;
	}

	if (chain->buffer_len - chain->misalign >= (size_t)size) {
		//第一个chain剩余空间足够
		size_t old_off = chain->off;
		buffer = chain->buffer + chain->misalign + chain->off;
		tmp = chain;
		tmp->off = size;
		size -= old_off;
		chain = chain->next;
	} else {
		if ((tmp = evbuffer_chain_new(size)) == NULL) {
			goto done;
		}
		buffer = tmp->buffer;
		tmp->off = size;
		buf->first = tmp;
	}

	last_with_data = *buf->last_with_datap;
	for (; chain != NULL && (size_t)size >= chain->off; chain = next) {
		next = chain->next;

		memcpy(buffer, chain->buffer + chain->misalign, chain->off);
		size -= chain->off;
		buffer += chain->off;
		if (chain == last_with_data)
			removed_last_with_data = 1;
		if (&chain->next == buf->last_with_datap)
			removed_last_with_datap = 1;

		evbuffer_chain_free(chain);
	}

	if (chain != NULL) {
		memcpy(buffer, chain->buffer + chain->misalign, size);
		chain->misalign += size;
		chain->off -= size;
	} else {
		buf->last = tmp;
	}

	tmp->next = chain;

	if (removed_last_with_data) {
		buf->last_with_datap = &buf->first;
	} else if (removed_last_with_datap) {
		if (buf->first->next && buf->first->next->off)
			buf->last_with_datap = &buf->first->next;
		else
			buf->last_with_datap = &buf->first;
	}

	result = (tmp->buffer + tmp->misalign);

done:
	return result;
}

#define EVBUFFER_CHAIN_MAX_AUTO_SIZE 4096
int evbuffer_add(struct evbuffer *buf, const void *data_in, size_t datlen)
{
	struct evbuffer_chain *chain, *tmp;
	const unsigned char *data = data_in;
	size_t remain, to_alloc;
	int result = -1;

	if (*buf->last_with_datap == NULL) {
		chain = buf->last;
	} else {
		chain = *buf->last_with_datap;
	}

	if (chain == NULL) {
		chain = evbuffer_chain_new(datlen);
		if (!chain)
			goto done;
		evbuffer_chain_insert(buf, chain);
	}

	remain = chain->buffer_len - (size_t)chain->misalign - chain->off;
	if (remain >= datlen) {
		memcpy(chain->buffer + chain->misalign + chain->off, data, datlen);
		chain->off += datlen;
		buf->total_len += datlen;
		buf->n_add_for_cb += datlen;
		goto out;
	}

	to_alloc = chain->buffer_len;
	if (to_alloc <= EVBUFFER_CHAIN_MAX_AUTO_SIZE/2)
		to_alloc <<= 1;
	if (datlen > to_alloc)
		to_alloc = datlen;
	tmp = evbuffer_chain_new(to_alloc);
	if (tmp == NULL)
		goto done;

	if (remain) {
		memcpy(chain->buffer + chain->misalign + chain->off,data, remain);
		chain->off += remain;
		buf->total_len += remain;
		buf->n_add_for_cb += remain;
	}

	data += remain;
	datlen -= remain;

	memcpy(tmp->buffer, data, datlen);
	tmp->off = datlen;
	evbuffer_chain_insert(buf, tmp);
	buf->n_add_for_cb += datlen;

out:
	evbuffer_invoke_callbacks_(buf);
	result = 0;
done:
	return result;
}

static int advance_last_with_data(struct evbuffer *buf)
{//移动last_with_datap
	int n = 0;
	struct evbuffer_chain **chainp = buf->last_with_datap;

	if (!*chainp)
		return 0;

	while ((*chainp)->next) {
		chainp = &(*chainp)->next;
		if ((*chainp)->off)
			buf->last_with_datap = chainp;
		++n;
	}
	return n;
}

static int get_n_bytes_readable_on_socket(evutil_socket_t fd)
{
#if defined(FIONREAD) && defined(_WIN32)
	unsigned long lng = EVBUFFER_MAX_READ;
	if (ioctlsocket(fd, FIONREAD, &lng) < 0)
		return -1;
	/* Can overflow, but mostly harmlessly. XXXX */
	return (int)lng;
#elif defined(FIONREAD)
	int n = EVBUFFER_MAX_READ;
	if (ioctl(fd, FIONREAD, &n) < 0)
		return -1;
	return n;
#else
	return EVBUFFER_MAX_READ;
#endif
}

int evbuffer_read(struct evbuffer *buf, evutil_socket_t fd, int howmuch)
{
	//struct evbuffer_chain **chainp;
	int n;
	int result;

	struct evbuffer_chain *chain;
	unsigned char *p;

	n = get_n_bytes_readable_on_socket(fd);
	if (n <= 0 || n > EVBUFFER_MAX_READ)
		n = EVBUFFER_MAX_READ;
	if (howmuch < 0 || howmuch > n)
		howmuch = n;

	if ((chain = evbuffer_expand_singlechain(buf, howmuch)) == NULL) {
		result = -1;
		goto done;
	}

	p = chain->buffer + chain->misalign + chain->off;

#ifndef _WIN32
	n = read(fd, p, howmuch);
#else
	n = recv(fd, p, howmuch, 0);
#endif


	if (n == -1) {
		result = -1;
		goto done;
	}
	if (n == 0) {
		result = 0;
		goto done;
	}

	chain->off += n;
	advance_last_with_data(buf);

	buf->total_len += n;
	buf->n_add_for_cb += n;

	evbuffer_invoke_callbacks_(buf);
	result = n;
done:
	return result;
}

int evbuffer_write_atmost(struct evbuffer *buffer, evutil_socket_t fd,ev_ssize_t howmuch)
{
	int n = -1;


	if (howmuch < 0 || (size_t)howmuch > buffer->total_len)
		howmuch = buffer->total_len;

	if (howmuch > 0) {
#ifdef _WIN32
		void *p = evbuffer_pullup(buffer, howmuch);
		n = send(fd, p, howmuch, 0);
#else
		void *p = evbuffer_pullup(buffer, howmuch);
		n = write(fd, p, howmuch);
#endif
	}

	if (n > 0)
		evbuffer_drain(buffer, n);

done:
	return (n);
}

static struct evbuffer_chain *evbuffer_expand_singlechain(struct evbuffer *buf, size_t datlen)
{
	struct evbuffer_chain *chain, **chainp;
	struct evbuffer_chain *result = NULL;

	chainp = buf->last_with_datap;
	
	if (*chainp && CHAIN_SPACE_LEN(*chainp) == 0)
		chainp = &(*chainp)->next;

	chain = *chainp;

	if (chain == NULL ) {
		goto insert_new;
	}

	if (CHAIN_SPACE_LEN(chain) >= datlen) {
		result = chain;
		goto ok;
	}

	if (chain->off == 0) {
		goto insert_new;
	}

	//剩余空间太少，则直接用新的chain
	if (CHAIN_SPACE_LEN(chain) < chain->buffer_len / 8 || chain->off > MAX_TO_COPY_IN_EXPAND ) {
		if (chain->next && CHAIN_SPACE_LEN(chain->next) >= datlen) {
			result = chain->next;
			goto ok;
		}
		else {
			goto insert_new;
		}
	}
	else {
		//new 新的chain，并复制已有的过去
		size_t length = chain->off + datlen;
		struct evbuffer_chain *tmp = evbuffer_chain_new(length);
		if (tmp == NULL)
			goto err;

		tmp->off = chain->off;
		memcpy(tmp->buffer, chain->buffer + chain->misalign,chain->off);
		result = *chainp = tmp;

		if (buf->last == chain)
			buf->last = tmp;

		tmp->next = chain->next;
		evbuffer_chain_free(chain);
		goto ok;
	}

insert_new:
	result = evbuffer_chain_insert_new(buf, datlen);
	if (!result)
		goto err;
ok:

err:
	return result;
}

void evbuffer_decref_and_unlock_(struct evbuffer *buffer)
{
	struct evbuffer_chain *chain, *next;

	for (chain = buffer->first; chain != NULL; chain = next) {
		next = chain->next;
		evbuffer_chain_free(chain);
	}
	evbuffer_remove_all_callbacks(buffer);

	mm_free(buffer);
}

ev_ssize_t evbuffer_copyout_from(struct evbuffer *buf, const struct evbuffer_ptr *pos,void *data_out, size_t datlen)
{
	/*XXX fails badly on sendfile case. */
	struct evbuffer_chain *chain;
	char *data = data_out;
	size_t nread;
	ev_ssize_t result = 0;
	size_t pos_in_chain;

	if (pos) {
		chain = pos->internal_.chain;
		pos_in_chain = pos->internal_.pos_in_chain;
		if (datlen + pos->pos > buf->total_len)
			datlen = buf->total_len - pos->pos;
	}
	else {
		chain = buf->first;
		pos_in_chain = 0;
		if (datlen > buf->total_len)
			datlen = buf->total_len;
	}


	if (datlen == 0)
		goto done;

	nread = datlen;

	while (datlen && datlen >= chain->off - pos_in_chain) {
		size_t copylen = chain->off - pos_in_chain;
		memcpy(data,chain->buffer + chain->misalign + pos_in_chain,copylen);
		data += copylen;
		datlen -= copylen;

		chain = chain->next;
		pos_in_chain = 0;
	}

	if (datlen) {
		memcpy(data, chain->buffer + chain->misalign + pos_in_chain,datlen);
	}

	result = nread;
done:
	return result;
}

struct evbuffer_cb_entry *evbuffer_add_cb(struct evbuffer *buffer, evbuffer_cb_func cb, void *cbarg)
{
	struct evbuffer_cb_entry *e;
	if (!(e = mm_calloc(1, sizeof(struct evbuffer_cb_entry))))
		return NULL;
	e->cb.cb_func = cb;
	e->cbarg = cbarg;
	//LIST_INSERT_HEAD(&buffer->callbacks, e, next);
	if (((e)->next.le_next = (&buffer->callbacks)->lh_first) != NULL)
		(&buffer->callbacks)->lh_first->next.le_prev = &(e)->next.le_next;
	(&buffer->callbacks)->lh_first = (e);
	(e)->next.le_prev = &(&buffer->callbacks)->lh_first;
	return e;
}

