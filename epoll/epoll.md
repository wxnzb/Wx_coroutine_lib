## 1
- 不使用 epoll_ctl() 的服务端
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOCKET_COUNT 10

int main() {
    int server_fd, new_fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[1024];

    // 创建监听套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    // 设置服务器地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定套接字
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 开始监听
    if (listen(server_fd, SOCKET_COUNT) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port 9999...\n");

    // 接受客户端连接
    while (1) {
        addr_len = sizeof(addr);
        new_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
        if (new_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected\n");

        // 读取数据
        ssize_t n = read(new_fd, buffer, sizeof(buffer));
        if (n > 0) {
            printf("Received: %s\n", buffer);
        }

        // 关闭连接
        close(new_fd);
    }

    // 关闭监听套接字
    close(server_fd);

    return 0;
}
```
- 他们对应的客户端代码都差不多
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char message[] = "Hello, Server!";
    char buffer[1024];

    // 创建客户端套接字
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        exit(1);
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 连接服务器
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    // 发送消息到服务器
    send(client_fd, message, sizeof(message), 0);

    // 接收服务器的响应
    ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
    if (n > 0) {
        buffer[n] = '\0';  // Null-terminate the received string
        printf("Server response: %s\n", buffer);
    }

    // 关闭客户端套接字
    close(client_fd);

    return 0;
}
```
- 疑问：客户端里 server_addr.sin_port = htons(SERVER_PORT); 这行，是不是 必须要和服务器的端口一样，比如都是 8080？
- ✅ 答案是：必须一样！也就是说：客户端这里设置的 server_addr.sin_port 必须是服务器 bind() 监听的端口。
## 2
- 对于这种我有个问题：那要是a客户端已经连接了这个服务器，但是没有发消息，这时候b客户端要连接呢，毕竟他是阻塞io呢
- ❓ 那会发生什么？
- ✅ 连接阶段（accept()）是不会被客户端 A 阻塞的！
- 当客户端 A 连接成功后，accept() 返回了 A 的连接 socket，服务器进入 处理 A 的阶段（例如开始 read()）。
- 如果服务器的代码是单线程、阻塞式处理，它正在 read() A 的数据，但 A 没发消息，于是 read() 阻塞。
- 💥 此时，服务器无法调用 accept() 来接收 B 的连接，所以：
- ❌ 客户端 B 无法连接成功，连接请求会排队或超时。

- 那应该怎样解决这个问题？
- 1.使用多线程，每个客户端连接一个线程，这样就可以同时处理多个客户端连接了。
- 2.使用epoll_ctl()

```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define SOCKET_COUNT 10

void *handle_client(void *arg) {
    int new_fd = *((int *)arg);
    free(arg);  // 释放传递的内存

    char buffer[1024];

    // 处理客户端的请求
    while (1) {
        ssize_t n = read(new_fd, buffer, sizeof(buffer));  // 阻塞直到有数据
        if (n > 0) {
            buffer[n] = '\0';  // Null-terminate the received string
            printf("Received from client: %s\n", buffer);

            // 回复客户端消息
            send(new_fd, "Message received", 16, 0);
        } else if (n == 0) {
            // 客户端关闭连接
            printf("Client disconnected\n");
            close(new_fd);
            break;
        } else {
            perror("read");
            break;
        }
    }

    return NULL;
}

int main() {
    int server_fd, new_fd;
    struct sockaddr_in addr;
    socklen_t addr_len;

    // 创建监听套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    // 设置服务器地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定套接字
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 开始监听
    if (listen(server_fd, SOCKET_COUNT) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port 9999...\n");

    // 接受客户端连接
    while (1) {
        addr_len = sizeof(addr);
        new_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
        if (new_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected\n");

        // 创建线程处理客户端
        pthread_t thread_id;
        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = new_fd;
        pthread_create(&thread_id, NULL, handle_client, fd_ptr);
        pthread_detach(thread_id);  // 让线程自我清理
    }

    // 关闭监听套接字
    close(server_fd);

    return 0;
}
```

- 或者就是main.cpp中的
- ## 3
- 客户端是怎么知道他要连接的是哪个服务器
- 客户端实例
```
int sock = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(9999);  // 服务器的端口号

// 把服务器 IP 地址转换为网络字节序
inet_pton(AF_INET, "192.168.1.100", &server_addr.sin_addr);

// 连接服务器
connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

```
- 客户端知道了服务器的 IP 是 192.168.1.100，端口是 9999，所以它就能发起连接。
- 当服务器调用：int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);这里的 client_addr 会被操作系统填上客户端的 IP 和端口号。

## 4
- 为啥events[i].data.fd==listen_fd就代表有新的客户端连接
  