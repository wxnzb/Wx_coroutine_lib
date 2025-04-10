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