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
- tickle 就是为了在“没有 IO 事件”的情况下，也能唤醒线程来处理协程、定时器、任务等其它调度事件。因为epoll_wait被唤醒的条件是有IO事件，但是有些比如定时器、任务等，不是IO事件，所以需要tickle来唤醒
## 10
- 为什么已经唤醒IO事件了，直接加入队，然后退出idel就行了，为什么还要调用tickle();
- A本来在epoll_wait阶段，但是被事件唤醒，执行triggerEvent(),在这个函数里面将事件加入队列，还需要调用tickle的原因是他可能唤醒的是另一个正在epoll_wait的线程对吗
## 10
-   // for (auto &i : m_threads)
        // {
        //     i->join();
        // }
        std::vector<std::shared_ptr<Thread>> thrs;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            thrs.swap(m_threads);
        }
        for(auto &i:thrs){
            i->join();
        }
- 上面的方法最好别用，因为m_threads是公共的，那样访问不安全
## 10
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
/这个/Scheduler::run()函数中，要是是新创建的线程，此时里面还没携程，需要先创建一个，然后取任务，要是这个任务指定的线程不是自己，就通过tickle();通知其他线程，然后通过携程来执行取出来要求是这个线程id的任务，要是线程上还没有任务，就主要运行idle_fiber->resume();	这个代码来执行空闲携程
//idle_fiber->resume()实际上std::shared_ptr<Fiber> idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle, this));他是这样初始化的，而 IOManager 是继承自 Scheduler 的，并且 重写了 idle() 函数，所以调用的其实是IOManager::idle()函数
//这个函数主要是调用epoll_wait 等待 IO 事件或定时器超时，然后执行相应那两个的回调函数，让出携程
//这里还是要详细讲一下，1.要是有不是IO事件的，例如所有的线程都在epoll_wait这步了，现在有个线程他将一个不是IO事件的任务加进队列了，此时调用tickle(),这个函数里面会他会通过m_tickleFds[1]写入，那么就会唤醒epoll_wait,通过read读并跳过，然后让出携程，就运行到schedule::run这里了2.IO事件唤醒epoll_wait的，就将事件通过triggerEvent函数主要调用scheduleLock，他主要是加入调度队列，要是队列为空，那么加入任务后应该立即通过tickle();唤醒别的正在epoll_wait的线程，或者是自己，最后通过 Fiber::GetThis()->yield();让出携程，此时他在Schedule::run中，他为什么又是遍历取出任务执行任务，执行完后又会在run这个循环中没事件就又进入idel

✅ 1. 所有线程都在 epoll_wait 中等待时：
场景：添加一个非 IO 的协程任务。

比如你手动调用了 scheduler.schedule(func) 或者定时器到期触发了一个回调。

这些新任务会被加入调度器的 m_tasks 队列中。

因为此时所有线程可能都在 epoll_wait 阻塞状态，所以：

需要主动调用 tickle()。

tickle() 往 m_tickleFds[1] 写入字节。

epoll_wait() 中注册了对 m_tickleFds[0] 的监听，会因此被唤醒。

被唤醒的线程读出这个字节（防止边沿触发一直存在），然后继续往下走，yield 出 idle 协程，让出 CPU 给调度器，从而进入 Scheduler::run() 主循环继续执行新任务。

✅ 2. IO 事件触发唤醒 epoll_wait：
比如 socket 收到数据，EPOLLIN 事件触发。

epoll_wait 返回，进入 idle() 的事件处理逻辑。

找到对应的 FdContext，调用 triggerEvent()。

triggerEvent() 会：

将协程或回调加入任务队列（scheduleLock()）。

如果之前任务队列是空的，会调用 tickle()，唤醒可能不是当前线程的其他空闲线程。

然后继续处理其他事件，最后调用 Fiber::GetThis()->yield()，把 idle 协程挂起，控制权交回 Scheduler::run()。

进入 Scheduler::run()，开始遍历任务队列，把刚才那个 IO 协程调度执行。

✅ 3. 为什么已经在当前线程里加入了任务，还要调用 tickle()？
因为当前线程可能不执行它自己调度的任务（比如已经有别的任务在处理、线程策略是优先其他线程）。

更重要的是，为了唤醒其他空闲线程一起来处理新任务（例如多核利用）。

所以 tickle 是一种“广播信号”：告诉其他正在 epoll_wait 的线程“有新任务了”，快醒来！

✅ 最终完整流程（你说得对，补几个关键点）：
所有线程空闲，进入 idle() 中 epoll_wait() 阻塞。

某线程添加新任务：

是 IO：系统唤醒 epoll_wait，处理事件。

不是 IO：需要主动 tickle() 唤醒。

epoll_wait 被唤醒后，idle 协程调用 Fiber::GetThis()->yield()。

控制权交回到 Scheduler::run()，从队列中取出任务执行。

任务执行完后，如果没有新任务，又会进入 idle()，循环继续。

可以把这个系统类比成一个饭店：

epoll_wait 就像厨师在厨房发呆（等订单）。

tickle() 就像服务员敲锅：有订单啦！

idle() 是厨师闲着时的状态。

schedule() + run() 是厨师做菜（执行任务）。
- 现在等于是一个线程里面有两个携程
- 现在的问题是你不是进入了新创建的线程的新创建的携程，他绑定的是idel,现在就在那里面
- 问题：新创建的线程的主携程是什么状态，还有为什么接下来结束的时候又是在主线程中
- m_runInScheduler这个到底具体指得是什么？假设为false就是换到主携程上去
- 主线程的新创建的携程上面绑定的是run
- 现在我知道了，现在在主携程要运行rin,那吗就要在新创建的携程上面，为啥上面那个实现打印1,而这个实现打印2

