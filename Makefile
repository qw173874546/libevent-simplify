all:
	gcc main.c evmap.c event.c select.c poll.c epoll.c evutil.c evutil_time.c buffer.c bufferevent.c bufferevent_ratelim.c bufferevent_sock.c -g