#include "hook.h"
#include<dlfcn.h>
#include"fiber.h"
#include"iomanager.h"
#include"iomanager.h"
#include <iostream>   // 用于 std::cerr 和 std::endl
#include <cstring>    // 用于 strerror()
#include <cerrno>     // 用于 errno 宏
#include "fd_manager.h"
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
    sylar::Singleton<sylar::FdManager>::GetInstance()->get(fd,true);
    return fd;
}
//是否取消定时器
struct timer_info{
    bool cancelled=0;
};
//这个函数还是比较难懂的
//首先要是没有打开hook，就直接调用原函数，然后找这个要连接的服务器fd的上下文，判断他是否存在，是否关闭，
//要是不是socket就直接调用原函数他会错误返回，用户自己设置了非阻塞就只调用原函数就行了，要是没设置，在获得fd的上下文的时候，也就是
//bool FdCtx::init()这个里面他会自己设置成非阻塞，然后把sysNonblock设置成true
//所以connect()` 在没连上目标服务器的时候，会立刻返回 `-1`，并设置 `errno = EINPROGRESS` —— 这就是**非阻塞 socket 正在连接中的正常表现**
//Sylar 利用协程挂起 + 写事件监听 + 定时器，就能完美模拟出“阻塞 connect 并支持超时”的行为
//上买就是他现在不是正在连接吗，为了让线程不等待，就先把他加入到事件中，然后让出线程，等他连接上，然后唤醒线程，然后判断是否超时，超时就返回错误，没超时就返回0
int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen,uint64_t timeout){
    if(!sylar::is_hook_enable()){
        return connect_f(sockfd,addr,addrlen);
    }
    std::shared_ptr<sylar::FdCtx> fdctx=sylar::Singleton<sylar::FdManager>::GetInstance()->get(sockfd);
    if(!fdctx){
        return -1;
    }
    if(fdctx->isClose()){
        errno=EBADF;//ebadf是什么
        return -1;
    }
    if(!fdctx->isSocket()){//要是不是套子节也就没必要connect了
        return connect_f(sockfd,addr,addrlen);//这里其实就返回错误了
    }
    if(fdctx->getUserNonblock()){//要是非阻塞的直接调用就行了，后面的是为了解决他没有设置成非阻塞，现在要解决他的阻塞问题
        return connect_f(sockfd,addr,addrlen);
    }
    int n=connect_f(sockfd,addr,addrlen);
    //连接成功
    if(n==0){
        return 0;
    }
    //连接失败，表明如果不是“正在连接中（EINPROGRESS）”，则返回错误
    if(errno!=EINPROGRESS||n!=-1){
        return n;
    }
    //那么下面这些就是connect在连接中
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info>wtinfo(tinfo);
    //设置定时器
    std::shared_ptr<sylar::Timer>  timer;
    //表明设置了计时器
    if(timeout!=(uint64_t)-1){
        iom->addConditionTimer(timeout,[iom,wtinfo,sockfd](){
         auto t=wtinfo.lock();
         if(!t){
            return;
         }
         t->cancelled=TIMEOUT;
         iom->cancelEvent(sockfd,sylar::IOManager::WRITE);
        },wtinfo);
    }
    //要是连接成功了
    int rt=iom->addEvent(sockfd,sylar::IOManager::Event(sylar::IOManager::WRITE));
    if(rt==0){
        //现在已经连接并且在监听了
        //让出携程
        sylar::Fiber::GetThis()->yeid();
        //被唤醒后，要是存在定时器，就删掉他
        if(timer){
            timer->cancel();
        }
        if(tinfo->cancelled){
            errno=tinfo->cancelled;
            return -1;
        }
    }
    else{
        //连接失败
        if(timer){
            timer->cancel();
            std::cerr<<"connect addEvent"<<sockfd<<" error"<<std::endl;
            return -1;
        }
    }
    //非阻塞 connect 的标准收尾检查流程，用 getsockopt(..., SO_ERROR) 获取连接最终状态
    int error=0;
    socklen_t len=sizeof(error);
    if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len)==-1){
        return -1;
    }
    if(error){
        errno=error;
        return -1;
    }
    return 0;
}
static uint64_t s_connect_timeout=-1;//表示不设置计时器
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    if(!sylar::is_hook_enable()){  
        return connect_with_timeout(sockfd,addr,addrlen,s_connect_timeout);
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
