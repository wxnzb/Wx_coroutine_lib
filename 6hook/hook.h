#ifndef _HOOK_H
#define _HOOK_H
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
namespace sylar
{
    bool is_hook_enable();
    void set_hook_enable(bool v);
}
extern "C"
{
    // 这些的实现时为了保存原函数的地址
    typedef int (*sleep_fun)(unsigned int seconds);
    sleep_fun sleep_f;
    typedef int (*usleep_fun)(useconds_t usec);
    usleep_fun usleep_f;
    typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
    nanosleep_fun nanosleep_f;

    // 套接字函数（socket 网络相关，sys/socket.h）
    typedef int (*socket_fun)(int domain, int type, int protocol);
    socket_fun socket_f;
    typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    connect_fun connect_f;
    typedef int (*accept_fun)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    accept_fun accept_f;
    typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
    send_fun send_f;
    typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
    sendto_fun sendto_f;
    typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
    sendmsg_fun sendmsg_f;
    typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
    recv_fun recv_f;
    typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    recvfrom_fun recvfrom_f;
    typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
    recvmsg_fun recvmsg_f;

    // 文件/套接字读写相关（unistd.h）
    typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
    read_fun read_f;
    typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
    readv_fun readv_f;
    typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
    write_fun write_f;
    typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
    writev_fun writev_f;

    // 关闭文件描述符（unistd.h）
    typedef int (*close_fun)(int fd);
    close_fun close_f;

    // 控制类函数
    typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
    fcntl_fun fcntl_f;
    typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
    ioctl_fun ioctl_f;
    typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
    getsockopt_fun getsockopt_f;
    typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
    setsockopt_fun setsockopt_f;
    // 下面这些是你自己实现的，在这里调用上面的这个原地址，就实现了自己重新定义函数的功能，那么系统看到相同的名字，怎样知道调用自己写的还是原函数呢？
    // 一个结构体，让它的构造函数去调用 hook_init()，然后创建一个静态对象。因为 C++ 规定静态对象会在 main() 之前构造
    unsigned int sleep(unsigned int seconds);
    int usleep(useconds_t usec);
    int nanosleep(const struct timespec *req, struct timespec *rem);

    // 套接字函数（socket 网络相关，sys/socket.h）
    int socket(int domain, int type, int protocol);
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    ssize_t send(int sockfd, const void *buf, size_t len, int flags);

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
    ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

    // 文件/套接字读写相关（unistd.h）
    ssize_t read(int fd, void *buf, size_t count);
    ssize_t write(int fd, const void *buf, size_t count);

    // 关闭文件描述符（unistd.h）
    int close(int fd);

    // 控制类函数
    int fcntl(int fd, int cmd, ... /* arg */);
    int ioctl(int fd, unsigned long request, ...);
    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
}
#endif
