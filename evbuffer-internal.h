
#ifndef EVBUFFER_INTERNAL_H_INCLUDED_
#define EVBUFFER_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"
#include "event_struct.h"
#include "buffer.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

struct evbuffer_cb_entry {
	struct {
		struct evbuffer_cb_entry *le_next;	
		struct evbuffer_cb_entry **le_prev;	
	} next;

	union {
		evbuffer_cb_func cb_func;
		//evbuffer_cb cb_obsolete;
	} cb;

	void *cbarg;
	//ev_uint32_t flags;
};

struct evbuffer_chain;
struct evbuffer {
	struct evbuffer_chain *first;
	struct evbuffer_chain *last;
	struct evbuffer_chain **last_with_datap;
	size_t total_len;
	size_t n_add_for_cb;
	size_t n_del_for_cb;

	struct evbuffer_cb_queue {
		struct evbuffer_cb_entry *lh_first;	
	} callbacks;
};

typedef ev_ssize_t ev_misalign_t;
struct evbuffer_chain {
	struct evbuffer_chain *next;
	size_t buffer_len;
	ev_misalign_t misalign;
	size_t off;
	unsigned char *buffer;
};

#define EVBUFFER_CHAIN_SIZE sizeof(struct evbuffer_chain)
#define MIN_BUFFER_SIZE	1024

void evbuffer_decref_and_unlock_(struct evbuffer *buffer);

#ifdef __cplusplus
}
#endif

#endif /* EVBUFFER_INTERNAL_H_INCLUDED_ */
