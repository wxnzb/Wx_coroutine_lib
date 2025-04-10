#include<sys/socket.h>
#include<netinet/in.h>
#include<cstring>
#include<sys/epoll.h>
#include<unistd.h>
#include<fcntl.h>
int main(){
    int listen_fd;
    //创建套子节
    if((listen_fd = socket(AF_INET,SOCK_STREAM,0))==-1){
        return -1;
    }
    int flags = fcntl(listen_fd, F_GETFL, 0);  // 获取当前的文件描述符标志
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
    //创建epoll实例
    int epoll_fd;
    if((epoll_fd=epoll_create(1))==-1){
        return -1;
    }
    //将套子节添加到epoll实例中
    struct epoll_event event;
    event.events=EPOLLIN;
    event.data.fd=listen_fd;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&event)==-1){
        return -1;
    }
    struct epoll_event events[2048];
    //循环监听epoll实例中的事件
    while(1){
      int epoll_count=epoll_wait(epoll_fd,events,2048,-1);
      for(int i=0;i<epoll_count;i++){
        if(events[i].data.fd==listen_fd){
            int conn_fd=accept(listen_fd,NULL,NULL);
            if(conn_fd==-1){
                return -1;
            }
             // 将 conn_fd 设置为非阻塞
             int flags = fcntl(conn_fd, F_GETFL, 0);  // 获取当前的文件描述符标志
            event.events=EPOLLIN;
            event.data.fd=conn_fd;
            if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,conn_fd,&event)==-1){
                return -1;
            }
        }else{
            char buf[1024];
            //处理数据
            int n=read(events[i].data.fd,buf,sizeof(buf)-1);
            if(n<=0){
                close(events[i].data.fd);
                continue;
            }else{
                const char* buffer="HTTP/1.1 200 OK\r\n"
                                           "Content-Type: text/plain\r\n"
                                           "Content-Length: 1\r\n"
                                           "Connection: keep-alive\r\n"
                                           "\r\n"
                                           "1";
                //这里不能用sizeof(buffer)，他是指针类型，64位机子上是8
                write(events[i].data.fd,buffer,strlen(buffer));
                epoll_ctl(epoll_fd,EPOLL_CTL_DEL,events[i].data.fd,NULL);
                close(events[i].data.fd);
            }
            
        }
      }
    }
    //处理事件
    close(listen_fd);
    close(epoll_fd);
    return 0;

}