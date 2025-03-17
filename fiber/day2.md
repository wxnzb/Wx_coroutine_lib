## 1
- std::atomic<uint64_t>s_fiber_id{0};它可以这样std::atomic<uint64_t>s_fiber_id=0；
- 在 C++11 及以上，std::atomic<T> 不能使用拷贝赋值（= 号初始化）进行直接赋值初始化
- std::atomic<T> 是不可拷贝的，因为它封装了底层原子操作，避免竞态条件。
- std::atomic<T> 必须使用大括号 {} 或小括号 () 进行初始化，因为：
- {} 叫列表初始化（推荐）
- () 叫直接初始化
- = 叫拷贝初始化，但 std::atomic 没有拷贝构造函数，无法使用 = 号初始化
## 2
- getcontext(&m_ctx)什么意思?
- getcontext(&m_ctx) 只是获取当前线程的上下文，保存到 m_ctx 结构体中
## 3
- 主协程和子协程有什么区别吗？？
- 主协程是当前线程的第一个协程，它通常不执行具体的任务，而是作为一个入口点，用于管理其他协程。
- 它并不是手动创建的，而是从当前线程的运行环境获取的，因此：它已经有上下文，不需makecontext  - 重新指定执行函数。其他普通协程运行结束后，会切换回主协程
## 4
- 	static uint64_t GetFiberId();他为啥是uint64_t，不是pid_t
- pid_t是进程id，不可以，为了保证协id足够多，就是那个了
## 5
- t_fiber->shared_from_this();要调用它需要包含什么
- #include<memory>
- 还有这个，不要忘了class MyClass : public std::enable_shared_from_this<MyClass>

