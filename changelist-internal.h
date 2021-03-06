
#ifndef CHANGELIST_INTERNAL_H_INCLUDED_
#define CHANGELIST_INTERNAL_H_INCLUDED_

#include "util.h"

struct event_change {
	evutil_socket_t fd;
	short old_events;
	ev_uint8_t read_change;
	ev_uint8_t write_change;
	ev_uint8_t close_change;
};

/* Flags for read_change and write_change. */

/* If set, add the event. */
#define EV_CHANGE_ADD     0x01
/* If set, delete the event.  Exclusive with EV_CHANGE_ADD */
#define EV_CHANGE_DEL     0x02
/* If set, this event refers a signal, not an fd. */
#define EV_CHANGE_SIGNAL  EV_SIGNAL
/* Set for persistent events.  Currently not used. */
#define EV_CHANGE_PERSIST EV_PERSIST
/* Set for adding edge-triggered events. */
#define EV_CHANGE_ET      EV_ET

/* The value of fdinfo_size that a backend should use if it is letting
 * changelist handle its add and delete functions. */
#define EVENT_CHANGELIST_FDINFO_SIZE sizeof(int)

#endif
