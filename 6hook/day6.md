## 1
- uint64_t m_recvTimeout = (uint64_t)-1;
- uint64_t 是一个无符号的 64 位整数类型。
- 范围是：0 到 2^64 - 1，也就是 0 到 18446744073709551615。
- (uint64_t)-1 是将 -1 强制转换为 uint64_t 类型。
- 因为 -1 在补码表示中，全是 1。
- 转成无符号数后，它就是 18446744073709551615，即 uint64_t 的最大值。

## 2
- #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
- name ## _f：使用的是 ## 拼接运算符，表示拼接标识符
- #name：使用的是 # 字符串化运算符，表示把参数转换成 字符串
  
## 3
- 就是要是休眠3秒，他运行iom->addTimer(3000ms, [fiber]() { iom->schedule(fiber); });，可能小于三秒，人则下面需要将携程挂起来，因为上一句已经加入到队列了
- ✅ 第一句：addTimer 只是安排 3 秒后执行 schedule(fiber)，相当于注册闹钟。
- ✅ 第二句：yield() 是主动挂起当前协程，交还 CPU 让别的协程/线程跑。
- ✅ 调度器 3 秒后执行 lambda，把 fiber 放入可运行队列；但是携程不一定三秒后能运行，因为在队列里面可能还需要排队
- ✅ 协程重新被调度运行，从 yield() 的位置继续往下执行。
## 4
- [fiber, iom]() {
    iom->scheduleLock(fiber, -1);
}
- lamada表达式
- 1. [fiber, iom] —— 捕获列表----我需要用到外部这两个变量 fiber 和 iom，请帮我 捕获进来
- 2.() —— 参数列表-----这里参数为空，表示这个 lambda 不接受任何外部传进来的参数。
- 3.{ ... } —— 函数体----表示：当这个 lambda 被调用的时候，就执行这句代码：
## 5
- iom->addTimer(seconds*1000, [fiber, iom](){iom->scheduleLock(fiber, -1);});这里为什么要*1000
- 这个 *1000 是因为 addTimer 这个函数 单位是毫秒（ms），而 sleep(seconds) 接收到的参数是 秒（s）。
## 6
- sylar::Singleton<sylar::FdManager>::GetInstance()->get(fd,true);解释一下这段代码
- sylar::Singleton<sylar::FdManager>这是一个单例模板类，作用是：管理 FdManager 的单例 —— 只会存在一个全局唯一的 FdManager 对象。
- .GetInstance()调用单例的获取方法，返回一个指向全局唯一 FdManager 实例的指针。
- ->get(fd, true)调用 FdManager 实例的 get() 方法。
- 问题：但是他找到fdctx好像也没用阿，他有没赋值给谁
- 确保这个 fd 对应的 FdCtx 被记录在 FdManager 中”，哪怕现在没用，后面用 read()、write()、close() 时就能查到了，如果已经有了，就直接返回，要是没有，就要创建
## 7
-  connect_with_timeout这个函数大概做的事情
// 这个函数主要用于 hook 掉 connect 实现“阻塞 connect + 支持超时”的模拟逻辑。
// 
// 1. 如果没有启用 hook（t_hook_enable 为 false），那就直接调用系统原生 connect。
// 
// 2. 否则进入 hook 模拟逻辑：
//    - 获取 fd 的上下文（FdCtx）
//    - 判断 fd 是否有效、是否关闭、是否是 socket。如果不是 socket 或已关闭，就返回错误或直接走原始逻辑。
//    - 如果用户手动设置了非阻塞（getUserNonblock()），也直接调用原始 connect，不做 hook。
// 
// 3. 如果用户没有设置非阻塞：
//    - Sylar 框架会在 FdCtx::init() 中，把 socket 设置为系统层非阻塞（fcntl 加上 O_NONBLOCK），
//    - 并将 m_sysNonblock 设置为 true，确保后续逻辑是基于非阻塞 socket 的行为。
//
// 4. 此时调用 connect()，如果连接未建立好，就会返回 -1，并设置 errno = EINPROGRESS。
//    - 这是非阻塞 socket 正在连接中的标准表现，表示“连接还在进行中”。
//
// 5. 为了模拟“阻塞 connect”，Sylar 做了以下操作：
//    - 将当前协程挂起（Fiber::yield），让出执行权（不阻塞线程）
//    - 向 IOManager 注册一个写事件监听（fd 可写 = 连接成功）
//    - 同时添加一个定时器，如果在 timeout_ms 毫秒内没有连接成功，就取消事件并设置 ETIMEDOUT。
//
// 6. 当 socket 可写时或超时事件触发，会唤醒当前协程：
//    - 如果超时了，返回错误（errno = ETIMEDOUT）
//    - 如果写事件唤醒，调用 getsockopt 检查是否真正连接成功（error == 0），是的话返回 0，否则设置 errno。
//
// ✅ 总结：
//    虽然 socket 是非阻塞的，但 Sylar 利用协程挂起 + epoll 写事件监听 + 定时器，模拟出一个“支持超时的阻塞 connect”，
//    用户感知上就是一个正常的阻塞 connect，底层却完全不阻塞线程，非常适合高并发协程调度。
## 8
- template<typename OriginFun,typename ... Args>
- static size_t io_do(int fd,OriginFun fun,const char* hook_fun_name,uint32_t event,int timeout_so,Args && ... args){}
- 什么用法
- ✅ 意思：
- 这是一段 函数模板定义，表示该函数适用于各种类型的参数：
- OriginFun：是一个函数类型或函数指针，表示我们要“hook”（钩住）原始调用的函数，比如 read() 或 recv()；
- typename... Args：表示任意数量的参数，使用的是C++11 的可变参数模板（variadic templates）；

- 🎯 举例说明：
- 如果你调用的是某个 socket 函数，比如：
- ssize_t recv(int sockfd, void *buf, size_t len, int flags);
- 那么：
- OriginFun 是：decltype(recv)（也可以是 ssize_t (*)(int, void*, size_t, int)）；
- Args... 是：int, void*, size_t, int
- 相当于另一种调用，connect_f(sockfd,addr,addrlen);这个hook.cc中也有写
  
## 9
- fun(fd, std::forward<Args>(args)...)那这个能详细解释一下吗，参是前面std::forward<Args>这个详细讲义下
- 是 C++ 中“完美转发” + 可变参数展开 的经典写法
- 这个是 完美转发（perfect forwarding） 的核心技巧。
- ✅ 完美转发的作用：
- 保留参数的“引用性”或“右值性”。
- 如果传入的是左值，就转发成左值；
- 如果传入的是右值，就转发成右值；
- 避免拷贝、重复构造等性能浪费。
  
## 10
- va_start(va,request);void* arg=va_arg(va,void*);解释一下
- 始化一个 va_list 变量（va），让它指向request 参数后面的第一个可变参数的位置。
- 从 va 当前指向的位置，取出一个类型为 void* 的参数，并把 va 推进到下一个变参的位置



