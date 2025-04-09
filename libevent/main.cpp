#include<sys/socket.h>
#include<netinet/in.h>
#include<cstring>
#include<sys/epoll.h>
#include<unistd.h>
#include<event2/event.h>
int main(){
    int listen_fd;
    //创建套子节
    if((listen_fd = socket(AF_INET,SOCK_STREAM,0))==-1){
        return -1;
    }
     // 解决 "address already in use" 错误
     int opt=1;
     setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //将套子节和相应的服务端绑定
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(8080);
    server_addr.sin_addr.s_addr=INADDR_ANY;
    if(bind(listen_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1){
        return -1;
    }
     //监听套子节
     if(listen(listen_fd,10)==-1){
        return -1;
     }
     struct event_base *base;
     //创建libevent库
     base=event_base_new();
     //创建监听事件
     struct event * listener_event;
     listener_event=event_new(base,listen_fd,EV_READ|EV_PERSIST,accept_cb,base)
}
void accept_cb(ev)