## 1
- // struct sockaddr_in client_addr;
    // socklen_t client_len=sizeof(client_addr);这个 struct sockaddr_storage client_addr;
    socklen_t client_len=sizeof(client_addr);有什么区别吗？
- struct sockaddr_in	专门用于 IPv4 地址（IP + 端口）
- struct sockaddr_storage	是个通用结构体，可以同时表示 IPv4 和 IPv6
## 2
 - //创建一个新的事件的结构体
        struct event *ev=event_new(NULL,-1,0,NULL,NULL);
        //将新的事件添加到libevent库中
        event_assign(ev,base,conn_fd,EV_READ|EV_PERSIST,read_cb,(void*)ev);
- 11------------------------------------
        listener_event=event_new(base,listen_fd,EV_READ|EV_PERSIST,accept_cb,base)
- 这样吗他两有什么区别吗
```
struct event *event_new(
    struct event_base *base,
    evutil_socket_t fd,
    short events,
    event_callback_fn callback,
    void *callback_arg
);
```
- 等于
```
struct event *ev = malloc(sizeof(struct event));
ev->fd = fd;
ev->base = base;
ev->events = events;
ev->callback = callback;
ev->callback_arg = callback_arg;
```
- tttttttttttttttttttttttttttttt
``` int event_assign(
    struct event *ev,
    struct event_base *base,
    evutil_socket_t fd,
    short events,
    event_callback_fn callback,
    void *callback_arg
);
```
- 等于
``` 
ev->fd = fd;
ev->base = base;
ev->events = events;
ev->callback = callback;
ev->callback_arg = callback_arg;
```
## 3
- epoll是非阻塞io这句话怎样来理解
- ❌ epoll 本身不是非阻塞 IO —— 它只负责“告诉你谁准备好了”
- ✅ 但它是 专门为非阻塞 IO 设计的高效通知机制
- 你把多个 socket 设置为 非阻塞（通过 fcntl(fd, F_SETFL, O_NONBLOCK)）
- 为什么会有多个socket,不应该就只有一个listen_fd吗，难道每个客户端的fd都是一个socket吗
- 你这个问题问得太对了！！！🔥🔥🔥
- 很多人一开始搞 socket 编程的时候都有这个疑惑：
- “不是已经有一个 listen_fd（监听套接字）了吗？为啥还会有多个 socket？”
- “难道每个客户端都对应一个 socket 吗？”
- 没错！答案就是：✅ 是的！每个客户端连接都会有自己独立的 socket（文件描述符 fd）！
- 具体要设置需要 int flags = fcntl(listen_fd, F_GETFL, 0);  // 获取当前的文件描述符标志这个才是真正的变成非阻塞