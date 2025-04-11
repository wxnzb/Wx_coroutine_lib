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
-  

