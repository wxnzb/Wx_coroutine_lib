#include "schedule.h"
#include "thread.h"

namespace sylar
{
    Scheduler *t_scheduler = nullptr;
    Scheduler::Scheduler(size_t threads, bool user_caller, const std::string &name) : m_name(name), m_user_caller(user_caller)
    {
        SetThis();
        Thread::SetName(name);
        // 使用主线程当作工作线程
        // 主线程也参与调度，意思是 主线程不仅仅是创建 Scheduler 并启动它，同时也会负责执行 Fiber 任务，而不是只是管理其他工作线程
        if (user_caller)
        {
            threads--;
            // 创建主携程
            Fiber::GetThis();
            // 创建调度携程，调度器的协程 m_schedulerFiber 会运行在主线程的 Fiber 环境中
            m_schedulerFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
            Fiber::SetScheduler(m_schedulerFiber.get());
            m_rootId = Thread::GetThreadId();
            m_threadIds.push_back(m_rootId);
        }
        m_threadNum = threads;
    }
    Scheduler::~Scheduler()
    {
        if (GetThis() == this)
        {
            t_scheduler == nullptr;
        }
    }
    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }
    void Scheduler::SetThis()
    {
        t_scheduler = this;
    }
    void Scheduler::start()
    {
        if (m_stopping)
        {
            return;
        }
        // 这里为什么要加锁,所以这个锁是哪里来的
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threads.resize(m_threadNum);
        for (size_t i = 0; i < m_threadNum; i++)
        {
            m_threads[i] = std::make_shared<Thread>(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i));
            m_threadIds.push_back(m_threads[i]->GetThreadId());
        }
    }
    void Scheduler::stop()
    {
        // 这里先进行判断一下
        if (m_stopping)
        {
            return;
        }
        m_stopping = true;
        for (size_t i = 0; i < m_threadNum; i++)
        {
            tickle();
        }
        // 感觉这里应该是if (m_usercalller),这里使唤醒主线程的主携程
        if (m_schedulerFiber)
        {
            tickle();
        }
        // 这里是唤醒调度器携程
        if (m_schedulerFiber)
        {
            m_schedulerFiber->resume();
        }
        for (auto &i : m_threads)
        {
            i->join();
        }
    }
    void Scheduler::tickle()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cond.notify_one();
    }
    void Scheduler::idle()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_stopping)
        {
            m_cond.wait(lock);
            lock.unlock();
            // 这里让出协程，让他执行其他协程，因为他被tickle唤醒了，就代表有任务了
            Fiber::GetThis()->yeid();
            lock.lock();
        }
    }
    void Scheduler::run()
    {
        // 1.线程 ID 相关
        int thread_id = Thread::GetThreadId();
        if (thread_id != m_rootId)
        {
            // 不是主线程的话要创建主协程
            Fiber::GetThis();
        }
        // 2.设置当前线程的调度器
        SetThis();
        // 3.只有非主线程需要创建主协程
        // 4. 创建空闲协程
        std::shared_ptr<Fiber> idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle, this));
        ScheduleTask task;
        // 5.开始任务调度循环
        while (true)
        {
            task.reset();
            bool tickle_me = false;
            // 找任务里面需要用到这个线程的
            {
                auto it = m_tasks.begin();
                while (it != m_tasks.end())
                {
                    if (it->thread_id == -1 || it->thread_id != thread_id)
                    {
                        // 说明有要运行的任务但是不是当前线程,说明还需要唤醒其他线程
                        tickle_me = true;
                        continue;
                    }
                    task = *it;
                    m_tasks.erase(it);
                    break;
                }
                tickle_me = tickle_me || (it != m_tasks.end());
            }
            if (tickle_me)
            {
                tickle();
            }
            if (task.fiber)
            {
                std::lock_guard<std::mutex> lock(task.fiber->m_mutex);
                task.fiber->resume();
                m_activeThreadCount--;
                task.reset();
            }
            // 要把他封装成一个协程才符合调度器
            //  if(task.cb){
            //      task.cb();
            //      m_activeThreadCount--;
            //      task.reset();
            //  }
            else if (task.cb)
            {
                std::shared_ptr<Fiber> cb_fiber = std::make_shared<Fiber>(task.cb, false);
                std::lock_guard<std::mutex> lock(cb_fiber->m_mutex);
                cb_fiber->resume();
                m_activeThreadCount--;
                task.reset();
            }
            else
            {
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    break;
                }
                m_idleThreadCount++;
                idle_fiber->resume();
                m_idleThreadCount--;
            }
        }
    }
    bool Scheduler::stopping()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stopping || m_tasks.empty() || m_activeThreadCount == 0;
    }
};
