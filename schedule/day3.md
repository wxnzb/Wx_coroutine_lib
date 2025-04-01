## 1
- idle（空闲）状态指的是 线程没有执行任何任务，正在等待新任务的状态。

## 2
- user_caller


- user_caller == true

- 主线程创建 自己的 Fiber（主协程）。
- 主线程创建 调度协程 m_schedulerFiber，它会运行 Scheduler::run() 进行任务调度。
- 主线程 ID 被加入 m_threadIds，表示它是一个调度线程。

- user_caller == false（主线程不参与调度）

- 主线程不会创建 m_schedulerFiber，而是创建多个 Thread，每个线程都有自己的调度协程。
- 主线程只是管理者，不会运行 Fiber 任务。
- 新的线程会执行 Scheduler::run()，每个线程都会创建自己的调度协程，并负责运行 Fiber 任务。

## 3
- m_schedulerFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
- 这里为什么都是false,也就是调度器运行子协程，完了后不会回到调度器而是回到主协程，这种就是非对称
```
如果 false：
任务 Fiber::yeid() 之后，会切回主协程（t_thread_fiber）。
主协程（t_thread_fiber）需要手动 resume() 让调度器继续运行，才能切换到下一个任务。
这样主协程始终是"中间层"，控制着任务调度的运行节奏。

如果 true：
任务 Fiber::yeid() 之后，会直接切回调度协程 m_schedulerFiber。
调度器（m_schedulerFiber）立即运行下一个任务，而无需回到主协程。
这样调度器完全自动运行，不依赖主协程的调

```
## 4
- // 创建主携程Fiber::GetThis();他和主线程有什么关系
```
这个调用会创建当前线程的主协程 t_thread_fiber。

这个协程不会主动执行任务，而是用于保存主线程的上下文，确保在 yield() 时可以切换回来。

主协程 = 主线程的入口点，它的作用就是在任务 yield() 时作为切换目标。

m_schedulerFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false))：

这个 Fiber 负责执行 Scheduler::run()，即真正的调度器逻辑。

这个 Fiber 是调度器专属的协程，不是主协程。

它运行在主线程的 Fiber 环境，但它本质上只是主线程上的一个普通 Fiber。

切换关系：

主线程默认在 t_thread_fiber（主协程）上运行。

当 m_schedulerFiber 启动时，主线程切换到 m_schedulerFiber，调度器开始执行。

任务 Fiber yield() 时，会切回 t_thread_fiber（主协程），然后由主线程决定是否继续运行 m_schedulerFiber。

m_schedulerFiber 继续调度下一个任务。
```

## 5
- Scheduler::run里面分类为啥上面两要加锁，第三个不会
- 如果多个线程同时尝试 resume() 同一个协程，可能会导致竞争条件
- idle_fiber 确实是一个协程，但它不会被多个线程同时调用，因此不需要加锁；每个线程运行 Scheduler::run() 时，会创建自己的 idle_fiber，所以不同线程的 idle_fiber 是独立的，互不影响

## 6
