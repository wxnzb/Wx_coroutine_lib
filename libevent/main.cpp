#include<sys/socket.h>
#include<netinet/in.h>
#include<cstring>
#include<sys/epoll.h>
#include<unistd.h>
#include<event2/event.h>
#include<event2/util.h>
void read_cb(evutil_socket_t conn_fd,short event,void *arg){
    struct event *ev=(struct event*)arg;
    char buf[1024];
            //处理数据
            int n=read(conn_fd,buf,sizeof(buf)-1);
            if(n<=0){
                //close(events[i].data.fd);
                event_free(ev);
            }else{
            buf[n]='\0';
            printf("接收到消息:%s\n",buf);
                const char* buffer="HTTP/1.1 200 OK\r\n"
                                           "Content-Type: text/plain\r\n"
                                           "Content-Length: 22\r\n"
                                           "Connection: keep-alive\r\n"
                                           "\r\n"
                                           "Hello wuxi,no give up\n";
                //这里不能用sizeof(buffer)，他是指针类型，64位机子上是8
                // write(events[i].data.fd,buffer,strlen(buffer));
                // epoll_ctl(epoll_fd,EPOLL_CTL_DEL,events[i].data.fd,NULL);
                // close(events[i].data.fd);
                send(conn_fd,buffer,strlen(buffer),0);
                close(conn_fd);
                event_free(ev);
}
}
void accept_cb(evutil_socket_t listen_fd,short event,void *arg){
    struct event_base *base=(struct event_base*)arg;
    // struct sockaddr_in client_addr;
    // socklen_t client_len=sizeof(client_addr);
    // int conn_fd=accept(listen_fd,(struct sockaddr*)&client_addr,&client_len);
    struct sockaddr_storage client_addr;
    socklen_t client_len=sizeof(client_addr);
    int conn_fd=accept(listen_fd,(struct sockaddr*)&client_addr,&client_len);
    if(conn_fd==-1){
        return ;
    }
    else{
        
        //创建一个新的事件的结构体
        struct event *ev=event_new(NULL,-1,0,NULL,NULL);
        //将新的事件添加到libevent库中
        event_assign(ev,base,conn_fd,EV_READ|EV_PERSIST,read_cb,(void*)ev);
        event_add(ev,NULL);
    }
}

int main(){
    int listen_fd;
    //创建套子节
    if((listen_fd = socket(AF_INET,SOCK_STREAM,0))==-1){
        return -1;
    }
    //将listen_fd设置成非阻塞
    evutil_make_socket_nonblocking(listen_fd);
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
     listener_event=event_new(base,listen_fd,EV_READ|EV_PERSIST,accept_cb,base);
     event_add(listener_event,NULL);
     //开始事件循环
     event_base_dispatch(base);
        //释放资源
        event_free(listener_event);
        event_base_free(base);
        close(listen_fd);

    }
