
#ifndef EVENT_INTERNAL_H_INCLUDED_
#define EVENT_INTERNAL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"
#include "event.h"

//表明是常规的读写事件，执行evcb_callback回调
#define EV_CLOSURE_EVENT 0
// 表明是信号，执行evcb_callback回调
//#define EV_CLOSURE_EVENT_SIGNAL 1
//表明是永久性的非信号事件，使用evcb_callback回调
#define EV_CLOSURE_EVENT_PERSIST 2
//表明是简单的回调，使用evcb_selfcb回调
//#define EV_CLOSURE_CB_SELF 3
//表明是结束型回调，使用evcb_cbfinalize回调
//#define EV_CLOSURE_CB_FINALIZE 4
//表明是结束事件，使用evcb_evfinalize回调
//#define EV_CLOSURE_EVENT_FINALIZE 5
//正在结束的事件后面应该释放；使用evcb_evfinalize回调
//#define EV_CLOSURE_EVENT_FINALIZE_FREE 6


struct eventop {
	const char *name;
	void *(*init)(struct event_base *);
	int(*add)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	int(*del)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	int(*dispatch)(struct event_base *, struct timeval *);
	void(*dealloc)(struct event_base *);
	int need_reinit;
	enum event_method_feature features;
	size_t fdinfo_len;
};

//正常情况下windows用hash表存fd，linux直接用fd作数组索引，这里不作优化统一用hash�?
struct event_map_entry;
struct event_io_map {                                     
	struct event_map_entry **hth_table;        //event_map_entry指针的数�?                                                                    
	unsigned hth_table_length;                 //数组长度                          
	unsigned hth_n_entries;					   //已插入的个数
	unsigned hth_load_limit;                   //插入的个数限制，超过了就扩容                        
	int hth_prime_idx;                         //当前容量索引                         
};

//event_callback链表
struct evcallback_list {
	struct event_callback *tqh_first;	
	struct event_callback **tqh_last;	
};

struct event_base {
	const struct eventop *evsel;	//io多路复用模型相关接口
	void *evbase;					//io多路复用模型内部数据结构

	struct event_io_map io;			//hash表结�?

	int event_count_active;			//当前active event�?
	int event_count_active_max;		//最大active event�?	

	struct evcallback_list *activequeues;		//evcallback_list数组
	int nactivequeues;							//evcallback_list数组长度
};

//忽略的io模型链表
struct event_config_entry {
	struct {
		struct event_config_entry *tqe_next;	
		struct event_config_entry **tqe_prev;	
	} next;
	const char *avoid_method;
};

struct event_config {
	struct event_configq {
		struct event_config_entry *tqh_first;	
		struct event_config_entry **tqh_last;	
	} entries;	//忽略的io模型链表
};

//用不�?
#define EVENT_DEL_NOBLOCK 0
#define EVENT_DEL_BLOCK 1
#define EVENT_DEL_AUTOBLOCK 2
#define EVENT_DEL_EVEN_IF_FINALIZING 3

int event_add_nolock_(struct event *ev,const struct timeval *tv, int tv_is_absolute);

void event_active_nolock_(struct event *ev, int res, short count);

int event_callback_activate_nolock_(struct event_base *, struct event_callback *);

int event_del_nolock_(struct event *ev, int blocking);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_INTERNAL_H_INCLUDED_ */