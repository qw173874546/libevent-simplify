

#include <string.h>
#include <stdlib.h>

#include "event.h"
#include "event_struct.h"
#include "util.h"
#include "bufferevent.h"
#include "bufferevent_struct.h"
#include "buffer.h"

#include "bufferevent-internal.h"
#include "mm-internal.h"
#include "event-internal.h"


#define MAX_SINGLE_READ_DEFAULT 16384
#define MAX_SINGLE_WRITE_DEFAULT 16384

static ev_ssize_t bufferevent_get_rlim_max_(struct bufferevent_private *bev, int is_write)
{
	ev_ssize_t max_so_far = is_write?bev->max_single_write:bev->max_single_read;
	return max_so_far;
}

ev_ssize_t bufferevent_get_read_max_(struct bufferevent_private *bev)
{
	return bufferevent_get_rlim_max_(bev, 0);
}

ev_ssize_t bufferevent_get_write_max_(struct bufferevent_private *bev)
{
	return bufferevent_get_rlim_max_(bev, 1);
}

int bufferevent_ratelim_init_(struct bufferevent_private *bev)
{
	bev->max_single_read = MAX_SINGLE_READ_DEFAULT;
	bev->max_single_write = MAX_SINGLE_WRITE_DEFAULT;

	return 0;
}
