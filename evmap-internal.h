
#ifndef EVMAP_INTERNAL_H_INCLUDED_
#define EVMAP_INTERNAL_H_INCLUDED_

#include "util.h"

struct event_base;
struct event;

void evmap_io_initmap_(struct event_io_map* ctx);

void evmap_io_clear_(struct event_io_map* ctx);

int evmap_io_add_(struct event_base *base, evutil_socket_t fd, struct event *ev);

int evmap_io_del_(struct event_base *base, evutil_socket_t fd, struct event *ev);

void evmap_io_active_(struct event_base *base, evutil_socket_t fd, short events);

void *evmap_io_get_fdinfo_(struct event_io_map *ctx, evutil_socket_t fd);

void evmap_delete_all_(struct event_base *base);

#endif /* EVMAP_INTERNAL_H_INCLUDED_ */
