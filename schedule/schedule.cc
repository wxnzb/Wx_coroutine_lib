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
        if (user_caller)
        {
            threads--;
            // 创建主携程
            Fiber::GetThis();
            // 创建调度携程
            m_schedulerFiber.reset(new Fiber(std::bind(&Scheduler::run, this)), 0, false);
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
    void Scheduler::start(){
        if(m_stopping){
            return;
        }
        //这里为什么要加锁,所以这个锁是哪里来的
        std::lock_guard<std::mutex>lock(m_mutex);
        m_threads.resize(m_threadNum);
        for(size_t i=0;i<m_threadNum;i++){
            m_threads[i]=std::make_shared<Thread>(std::bind(&Scheduler::run,this),m_name+"_"+std::to_string(i));
            m_threadIds.push_back(m_threads[i]->GetThreadId());
        }
    }
    void Scheduler::stop(){
        //这里先进行判断一下
        m_stopping=true;
        for(size_t i=0;i<m_threadNum;i++){
            tickle();
        }
        if(m_schedulerFiber){
            tickle();
        }
        if(m_schedulerFiber){
            m_schedulerFiber->resume();
        }
        for(auto &i:m_threads){
            i->join();
        }
    }
    void Scheduler::tickle(){
    
    }
};
