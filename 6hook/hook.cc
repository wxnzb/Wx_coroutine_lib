#include "hook.h"
#include<dlfcn.h>
#include"fiber.h"
#include"iomanager.h"
#include"iomanager.h"
#include <iostream>   // 用于 std::cerr 和 std::endl
#include <cstring>    // 用于 strerror()
#include <cerrno>     // 用于 errno 宏
#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(write)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)
// 标准库的原函数长这样
// 睡眠类函数（unistd.h）
// unsigned int sleep(unsigned int seconds);------------------------------------让当前线程休眠 seconds 秒。
// int usleep(useconds_t usec); ------------------------------------------------精确到微秒的睡眠。
// int nanosleep(const struct timespec *req, struct timespec *rem);-------------精确到纳秒的休眠。

// 套接字函数（socket 网络相关，sys/socket.h）
//  int socket(int domain, int type, int protocol);-----------------------------创建一个套接字。
//  int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);----连接远程主机
//  int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);----------接受一个客户端连接
//  ssize_t send(int sockfd, const void *buf, size_t len, int flags);---------------------------------------------------------用于 socket 的写操作，适配 TCP/UDP 和控制选项

// ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen);---用于 socket 的写操作，适配 TCP/UDP 和控制选项
// ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);----------------------------------------------------------用于 socket 的写操作，适配 TCP/UDP 和控制选项
// ssize_t recv(int sockfd, void *buf, size_t len, int flags);-----------------从 socket 接收数据
// ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);--------------用于 UDP 或复杂结构的数据接收
// ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);----------------------------------------------------------------用于 UDP 或复杂结构的数据接收

// 文件/套接字读写相关（unistd.h）
//  ssize_t read(int fd, void *buf, size_t count);-----------------------------从文件描述符中读取数据
//  ssize_t write(int fd, const void *buf, size_t count);----------------------写数据到文件描述符

// 关闭文件描述符（unistd.h）
// int close(int fd);

// 控制类函数
// int fcntl(int fd, int cmd, ... /* arg */);fcntl（fcntl.h）-------------------控制文件描述符，比如设置非阻塞等
// int ioctl(int fd, unsigned long request, ...);ioctl（sys/ioctl.h）-----------对设备执行控制操作（底层复杂控制）
// int getsockopt(int sockfd, int level, int optname,void *optval, socklen_t *optlen);-------------------------------------------获取或设置 socket 选项（如超时、重用端口等）
// int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen);-------------------------------------获取或设置 socket 选项（如超时、重用端口等）
namespace sylar
{
    static thread_local bool t_hook_enable;
    bool is_hook_enable()
    {
        return t_hook_enable;
    }
    void set_hook_enable(bool v)
    {
        t_hook_enable = v;
    }
    void InitHook()
    {
        // 其实我现在感觉下面这个没用
        static bool hook_init = false;
        if (hook_init)
        {
            return;
        }
        hook_init = true;
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
#undef XX
    }
    struct HookIniter
    {
        HookIniter()
        {
            InitHook();
        }
        static struct HookIniter s_hookiniter;
    };
}
unsigned int sleep(unsigned int seconds){//秒
    if(!sylar::is_hook_enable()){
        return sleep_f(seconds);
    }
    std::shared_ptr<sylar::Fiber> fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    iom->addTimer(seconds*1000,[fiber,iom](){//毫秒
        iom->scheduleLock(fiber,-1);
    });
    fiber->yeid();
	return 0;
}
int usleep(useconds_t usec){//微妙
    if(!sylar::is_hook_enable()){
        usleep_f(usec);
    }
    std::shared_ptr<sylar::Fiber> fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    iom->addTimer(usec/1000,[fiber,iom](){//毫秒
        iom->scheduleLock(fiber,-1);
    });
    fiber->yeid();
}
int nanosleep(const struct timespec *req, struct timespec *rem){
    if(!sylar::is_hook_enable()){
        return nanosleep_f(req,rem);
    }
    std::shared_ptr<sylar::Fiber> fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    auto ms=req->tv_sec*1000+req->tv_nsec/1000000;
    iom->addTimer(ms,[fiber,iom](){//毫秒
        iom->scheduleLock(fiber,-1);
    });
    fiber->yeid();
}

// 套接字函数（socket 网络相关，sys/socket.h）
int socket(int domain, int type, int protocol){
    if(!sylar::is_hook_enable()){
        return socket_f(domain,type,protocol);
    }
    int fd=socket_f(domain,type,protocol);
    if(fd==-1){
        std::cerr<<"socket error"<<strerror(errno)<<std::endl;
        return -1;
    }
}
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    if(!sylar::is_hook_enable()){  
        return connect_f(sockfd,addr,addrlen);
    }
}
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    if(!sylar::is_hook_enable()){   
        return accept_f(sockfd,addr,addrlen);
    }
}
ssize_t send(int sockfd, const void *buf, size_t len, int flags){
    if(!sylar::is_hook_enable()){
        return send_f(sockfd,buf,len,flags);
    }
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){
    if(!sylar::is_hook_enable()){           
        return sendto_f(sockfd,buf,len,flags,dest_addr,addrlen);
    }
}
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    if(!sylar::is_hook_enable()){
        return sendmsg_f(sockfd,msg,flags);
    }
}
ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    if(!sylar::is_hook_enable()){
        return recv_f(sockfd,buf,len,flags);
    }
}
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    if(!sylar::is_hook_enable()){
        return recvfrom_f(sockfd,buf,len,flags,src_addr,addrlen);
    }
}
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    if(!sylar::is_hook_enable()){       
        return recvmsg_f(sockfd,msg,flags);
    }
}

// 文件/套接字读写相关（unistd.h）
ssize_t read(int fd, void *buf, size_t count){
    if(!sylar::is_hook_enable()){
        return read_f(fd,buf,count);}     
    }
ssize_t write(int fd, const void *buf, size_t count){
    if(!sylar::is_hook_enable()){
        return write_f(fd,buf,count);
    }
}

// 关闭文件描述符（unistd.h）
int close(int fd){
    if(!sylar::is_hook_enable()){
        return close_f(fd);
    }
}

// 控制类函数
int fcntl(int fd, int cmd, ... /* arg */){
    if(!sylar::is_hook_enable()){
    return fcntl_f(fd,cmd);
    }
}
int ioctl(int fd, unsigned long request, ...){
    if(!sylar::is_hook_enable()){
        return ioctl_f(fd,request);
    }
}
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen){
    if(!sylar::is_hook_enable()){       
        return getsockopt_f(sockfd,level,optname,optval,optlen);
    }
}
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen){
    if(!sylar::is_hook_enable()){   
        return setsockopt_f(sockfd,level,optname,optval,optlen);
    }
}
