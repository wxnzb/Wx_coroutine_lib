## 1
- epoll_create(5000) 设置了可以处理最多 5000 个事件，这里确实是指 可以处理最多 5000 个文件描述符，每个文件描述符可以有多个事件（比如读事件和写事件）。
## 2
IOManager* IOManager::GetThis(){
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }
- dynamic_cast<Derived*>(base_ptr) 是用来把一个 基类指针转换为子类指针 的安全方式
- 我拿到一个当前线程的调度器对象（Scheduler），但我知道它其实可能是一个 IOManager，于是我尝试把它强转回来
- 和static_cast的区别
- 用 static_cast 时，你必须非常确定 指针所指的对象类型。
- 而 dynamic_cast 是一种 带保护机制的类型转换，尤其适合多态下的父子类转换。
- 在 IOManager::GetThis() 中，作者不确定 Scheduler::GetThis() 返回的是不是 IOManager，所以用了 dynamic_cast，这是合理且安全的
## 3
-  epoll_event  ev;
        ev.events=EPOLLET|event|fd_ctx->events;
        ev.data.ptr=fd_ctx;这里ev.data.ptr=fd_ctx;这一行的作用是什么
-  在事件触发时，我们能知道这个事件是哪个 fd（文件描述符）触发的，并能快速访问它的上下文信息。
-  为什么不用 ev.data.fd = fd;？
-  因为 fd 只是个数字，拿到后还得去查找对应的 FdContext，多一步。而直接传 ptr 是最快的方式，一步到位，常见于高性能服务框架（像 libevent、libev、muduo 等）。
## 4
-  fd_ctx->events=Event(fd_ctx->events&~event);
        FdContext::EventContext& ctx=fd_ctx->getEventContext(event);
        fd_ctx->resetEventContext(ctx);这里不是都把event去掉了吗，为什么通过fd_ctx->getEventContext(event);还可以找到
- event 已经被从监听列表中去掉，但 getEventContext(event) 是根据参数来返回引用，并不依赖 fd_ctx->events 当前的状态，因此能正常工作。
## 5
- events = Event(events & ~event);为什么这里还要把他强制类型转换
- enum Event { NONE = 0, READ = 1, WRITE = 2 };
- Event e = READ;
- e = e & ~READ;  // ❌ 报错！因为 e & ~READ 的结果是 int，不能直接赋给 enum Event
- 所以你需要显式地告诉编译器：“嘿，我知道这结果是 int，但请你把它当作 Event 枚举值来处理”：
- e = (Event)(e & ~READ);  // ✅ OK
## 6
- delete all events
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events   = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);这里为什么是   epevent.events   = 0;，不应该是fd的所有事件吗，这样才可以删
- 因为 EPOLL_CTL_DEL 根本不关心 events 字段，删除的不是事件而是整个fd
- 那是不是+和-都是关于fd的，与事件无关，要是想修改一个fd里面的事件，就要用修改
## 7
- 那就是epoll_wait的时候，他等待很多，其中就包含m_tickleFds[0],要想唤醒epoll就需要在m_tickleFds[1]里面写入对吗
- epoll_wait(m_epfd, events.get(), MAX_EVENTS, timeout);
- 这个调用会阻塞，直到：
- 有被监听的 fd 变成就绪状态（比如可读/可写）；
- 或者超时；
- 或者被一个“tickle”事件唤醒。
- 其中被监听的 fd 列表中，就特意加了 m_tickleFds[0] —— 这是 tickle 的 唤醒管道读端。
- ## 8
- std::atomic<size_t> m_pendingEventCount={0};
        //int m_pendingEventCount=0;
- 为什么用上面这个，不用下面这个
  
## 9
- 整个工作流程
//1.IOManager manager(2);
//IOManager::IOManager(size_t threads, bool use_caller, const std::string &name): Scheduler(threads, use_caller, name), TimerManager()
//这句会先运行Scheduler(threads, use_caller, name)
//这个里面因为默认use_caller，也就是主线程创建一个携程，并绑定 Scheduler::run()函数，线程数-1
//然后运行IOManager::IOManager(size_t threads, bool use_caller, const std::string &name):
//他主要是创建epoll并等待读事件，接着start函数
//运行void Scheduler::start()函数
//根据线程数创建每一个线程并绑定 Scheduler::run()函数
//在创建新线程是，他会自动调用新线程的构造函数，运行Thread::Thread(std::function<void()> cb, const std::string &name)
//关键是他里面这行代码： int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);他主要是创建一个线程并运行绑定的Thread::run
//在Thread::run这个函数中，主要运行cb();也就是Scheduler::run()函数
//Scheduler::run()函数中，要是是新创建的线程，此时里面还没携程，需要先创建一个，然后取任务，要是这个任务指定的线程不是自己，就通过tickle();通知其他线程，然后通过携程来执行取出来要求是这个线程id的任务，要是这个线程上还没有任务，就主要运行idle_fiber->resume();	这个代码来执行空闲携程
//idle_fiber->resume()实际上std::shared_ptr<Fiber> idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle, this));他是这样初始化的，而 IOManager 是继承自 Scheduler 的，并且 重写了 idle() 函数，所以调用的其实是IOManager::idle()函数
//这个函数主要是调用epoll_wait 等待 IO 事件或定时器超时



