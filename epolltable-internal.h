
#ifndef EPOLLTABLE_INTERNAL_H_INCLUDED_
#define EPOLLTABLE_INTERNAL_H_INCLUDED_

#include <sys/epoll.h>

//以下标志填入1-9位，EV_CLOSED|EV_READ|EV_WRITE|EV_CHANGE_ADD|EV_CHANGE_DEL|EV_CHANGE_ADD|EV_CHANGE_DEL|EV_CHANGE_ADD|EV_CHANGE_DEL
#define EPOLL_OP_TABLE_INDEX(c) \
	(   (((c)->close_change&(EV_CHANGE_ADD|EV_CHANGE_DEL))) |		\
	    (((c)->read_change&(EV_CHANGE_ADD|EV_CHANGE_DEL)) << 2) |	\
	    (((c)->write_change&(EV_CHANGE_ADD|EV_CHANGE_DEL)) << 4) |	\
	    (((c)->old_events&(EV_READ|EV_WRITE)) << 5) |		\
	    (((c)->old_events&(EV_CLOSED)) << 1)				\
	    )


static const struct operation {
	int events;
	int op;
} epoll_op_table[] = {
	/* old=  0, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old=  0, write:  0, read:  0, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:  0, read:  0, close:del */
	{ EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  0, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  0, write:  0, read:add, close:  0 */
	{ EPOLLIN, EPOLL_CTL_ADD },
	/* old=  0, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:  0, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_ADD },
	/* old=  0, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old=  0, write:  0, read:del, close:  0 */
	{ EPOLLIN, EPOLL_CTL_DEL },
	/* old=  0, write:  0, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:  0, read:del, close:del */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  0, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old=  0, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  0, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old=  0, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old=  0, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  0, write:add, read:  0, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:  0, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:  0, close:del */
	{ EPOLLOUT, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  0, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old=  0, write:add, read:del, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_ADD },
	/* old=  0, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old=  0, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  0, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old=  0, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old=  0, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  0, write:del, read:  0, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_DEL },
	/* old=  0, write:del, read:  0, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:del, read:  0, close:del */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  0, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  0, write:del, read:add, close:  0 */
	{ EPOLLIN, EPOLL_CTL_ADD },
	/* old=  0, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_ADD },
	/* old=  0, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old=  0, write:del, read:del, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_DEL },
	/* old=  0, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_ADD },
	/* old=  0, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  0, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old=  0, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  0, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old=  0, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old=  0, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  0, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old=  0, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old=  0, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old=  0, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  0, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old=  0, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old=  0, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old=  0, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old=  0, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old=  0, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old=  0, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old=  0, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old=  0, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  0, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old=  0, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old=  0, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  r, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old=  r, write:  0, read:  0, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:  0, read:  0, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  r, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  r, write:  0, read:add, close:  0 */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  r, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:  0, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  r, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old=  r, write:  0, read:del, close:  0 */
	{ EPOLLIN, EPOLL_CTL_DEL },
	/* old=  r, write:  0, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:  0, read:del, close:del */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  r, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old=  r, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  r, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old=  r, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old=  r, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  r, write:add, read:  0, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:  0, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:  0, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  r, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old=  r, write:add, read:del, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  r, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old=  r, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  r, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old=  r, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old=  r, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  r, write:del, read:  0, close:  0 */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  r, write:del, read:  0, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:del, read:  0, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  r, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  r, write:del, read:add, close:  0 */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  r, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  r, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old=  r, write:del, read:del, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_DEL },
	/* old=  r, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  r, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  r, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old=  r, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  r, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old=  r, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old=  r, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  r, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old=  r, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old=  r, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old=  r, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  r, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old=  r, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old=  r, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old=  r, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old=  r, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old=  r, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old=  r, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old=  r, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old=  r, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  r, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old=  r, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old=  r, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  w, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old=  w, write:  0, read:  0, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:  0, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  w, write:  0, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old=  w, write:  0, read:del, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old=  w, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  w, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old=  w, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old=  w, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  w, write:add, read:  0, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:  0, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:  0, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  w, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old=  w, write:add, read:del, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  w, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old=  w, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  w, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old=  w, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old=  w, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  w, write:del, read:  0, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_DEL },
	/* old=  w, write:del, read:  0, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:del, read:  0, close:del */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  w, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  w, write:del, read:add, close:  0 */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  w, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  w, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old=  w, write:del, read:del, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_DEL },
	/* old=  w, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  w, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  w, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old=  w, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  w, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old=  w, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old=  w, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  w, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old=  w, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old=  w, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old=  w, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  w, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old=  w, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old=  w, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old=  w, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old=  w, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old=  w, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old=  w, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old=  w, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old=  w, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  w, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old=  w, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old=  w, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= rw, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old= rw, write:  0, read:  0, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:  0, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old= rw, write:  0, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old= rw, write:  0, read:del, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old= rw, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= rw, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old= rw, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old= rw, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= rw, write:add, read:  0, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:  0, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:  0, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old= rw, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old= rw, write:add, read:del, close:  0 */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= rw, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old= rw, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= rw, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old= rw, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old= rw, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= rw, write:del, read:  0, close:  0 */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= rw, write:del, read:  0, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:del, read:  0, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= rw, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old= rw, write:del, read:add, close:  0 */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= rw, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= rw, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old= rw, write:del, read:del, close:  0 */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_DEL },
	/* old= rw, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= rw, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old= rw, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old= rw, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= rw, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old= rw, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old= rw, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= rw, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old= rw, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old= rw, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old= rw, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old= rw, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old= rw, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old= rw, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old= rw, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old= rw, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old= rw, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old= rw, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old= rw, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old= rw, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= rw, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old= rw, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old= rw, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  c, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old=  c, write:  0, read:  0, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:  0, read:  0, close:del */
	{ EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  c, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  c, write:  0, read:add, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:  0, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  c, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old=  c, write:  0, read:del, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:  0, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:  0, read:del, close:del */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  c, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old=  c, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  c, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old=  c, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old=  c, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  c, write:add, read:  0, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:  0, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:  0, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  c, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old=  c, write:add, read:del, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=  c, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old=  c, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  c, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old=  c, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old=  c, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  c, write:del, read:  0, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:del, read:  0, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:del, read:  0, close:del */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  c, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  c, write:del, read:add, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=  c, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old=  c, write:del, read:del, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=  c, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=  c, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old=  c, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  c, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old=  c, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old=  c, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=  c, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old=  c, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old=  c, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old=  c, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old=  c, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old=  c, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old=  c, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old=  c, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old=  c, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old=  c, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old=  c, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old=  c, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old=  c, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=  c, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old=  c, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old=  c, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cr, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old= cr, write:  0, read:  0, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:  0, read:  0, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= cr, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cr, write:  0, read:add, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:  0, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= cr, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old= cr, write:  0, read:del, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:  0, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:  0, read:del, close:del */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old= cr, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old= cr, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cr, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old= cr, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old= cr, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cr, write:add, read:  0, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:  0, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:  0, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cr, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old= cr, write:add, read:del, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cr, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old= cr, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cr, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old= cr, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old= cr, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cr, write:del, read:  0, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:  0, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:  0, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cr, write:del, read:add, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old= cr, write:del, read:del, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cr, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old= cr, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old= cr, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cr, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old= cr, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old= cr, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cr, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old= cr, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old= cr, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old= cr, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cr, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old= cr, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old= cr, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old= cr, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old= cr, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old= cr, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old= cr, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old= cr, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old= cr, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cr, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old= cr, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old= cr, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cw, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old= cw, write:  0, read:  0, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:  0, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cw, write:  0, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old= cw, write:  0, read:del, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cw, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old= cw, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cw, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old= cw, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old= cw, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cw, write:add, read:  0, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:  0, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:  0, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cw, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old= cw, write:add, read:del, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old= cw, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old= cw, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cw, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old= cw, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old= cw, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cw, write:del, read:  0, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:del, read:  0, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:del, read:  0, close:del */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old= cw, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cw, write:del, read:add, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old= cw, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old= cw, write:del, read:del, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old= cw, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old= cw, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old= cw, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cw, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old= cw, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old= cw, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old= cw, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old= cw, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old= cw, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old= cw, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old= cw, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old= cw, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old= cw, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old= cw, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old= cw, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old= cw, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old= cw, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old= cw, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old= cw, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old= cw, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old= cw, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old= cw, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=crw, write:  0, read:  0, close:  0 */
	{ 0, 0 },
	/* old=crw, write:  0, read:  0, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:  0, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:  0, close:xxx */
	{ 0, 255 },
	/* old=crw, write:  0, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:add, close:xxx */
	{ 0, 255 },
	/* old=crw, write:  0, read:del, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=crw, write:  0, read:del, close:xxx */
	{ 0, 255 },
	/* old=crw, write:  0, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=crw, write:  0, read:xxx, close:add */
	{ 0, 255 },
	/* old=crw, write:  0, read:xxx, close:del */
	{ 0, 255 },
	/* old=crw, write:  0, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=crw, write:add, read:  0, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:  0, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:  0, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:  0, close:xxx */
	{ 0, 255 },
	/* old=crw, write:add, read:add, close:  0 */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:add, close:add */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:add, close:del */
	{ EPOLLIN|EPOLLOUT, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:add, close:xxx */
	{ 0, 255 },
	/* old=crw, write:add, read:del, close:  0 */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:del, close:add */
	{ EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:del, close:del */
	{ EPOLLOUT, EPOLL_CTL_MOD },
	/* old=crw, write:add, read:del, close:xxx */
	{ 0, 255 },
	/* old=crw, write:add, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=crw, write:add, read:xxx, close:add */
	{ 0, 255 },
	/* old=crw, write:add, read:xxx, close:del */
	{ 0, 255 },
	/* old=crw, write:add, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=crw, write:del, read:  0, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:  0, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:  0, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:  0, close:xxx */
	{ 0, 255 },
	/* old=crw, write:del, read:add, close:  0 */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:add, close:add */
	{ EPOLLIN|EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:add, close:del */
	{ EPOLLIN, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:add, close:xxx */
	{ 0, 255 },
	/* old=crw, write:del, read:del, close:  0 */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:del, close:add */
	{ EPOLLRDHUP, EPOLL_CTL_MOD },
	/* old=crw, write:del, read:del, close:del */
	{ EPOLLIN|EPOLLOUT|EPOLLRDHUP, EPOLL_CTL_DEL },
	/* old=crw, write:del, read:del, close:xxx */
	{ 0, 255 },
	/* old=crw, write:del, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=crw, write:del, read:xxx, close:add */
	{ 0, 255 },
	/* old=crw, write:del, read:xxx, close:del */
	{ 0, 255 },
	/* old=crw, write:del, read:xxx, close:xxx */
	{ 0, 255 },
	/* old=crw, write:xxx, read:  0, close:  0 */
	{ 0, 255 },
	/* old=crw, write:xxx, read:  0, close:add */
	{ 0, 255 },
	/* old=crw, write:xxx, read:  0, close:del */
	{ 0, 255 },
	/* old=crw, write:xxx, read:  0, close:xxx */
	{ 0, 255 },
	/* old=crw, write:xxx, read:add, close:  0 */
	{ 0, 255 },
	/* old=crw, write:xxx, read:add, close:add */
	{ 0, 255 },
	/* old=crw, write:xxx, read:add, close:del */
	{ 0, 255 },
	/* old=crw, write:xxx, read:add, close:xxx */
	{ 0, 255 },
	/* old=crw, write:xxx, read:del, close:  0 */
	{ 0, 255 },
	/* old=crw, write:xxx, read:del, close:add */
	{ 0, 255 },
	/* old=crw, write:xxx, read:del, close:del */
	{ 0, 255 },
	/* old=crw, write:xxx, read:del, close:xxx */
	{ 0, 255 },
	/* old=crw, write:xxx, read:xxx, close:  0 */
	{ 0, 255 },
	/* old=crw, write:xxx, read:xxx, close:add */
	{ 0, 255 },
	/* old=crw, write:xxx, read:xxx, close:del */
	{ 0, 255 },
	/* old=crw, write:xxx, read:xxx, close:xxx */
	{ 0, 255 },
};

#endif
