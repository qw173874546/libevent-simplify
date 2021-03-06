# libevent-simplify
libevent精简，为了方便了解代码结构。

去掉了线程，信号，时间等，仅保留套接字部分，去掉了错误处理和提示。
代码基本只作删除，不改动，大部分宏作了展开。
io模型只保留windows的select,linux的select,poll,epoll。
不同系统修改event-config.h编译。
main.c为测试用。
