#include "fiber.h"
#include <memory>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <assert.h>
static bool debug = false;
// 实现了主协程和子协程之间的切换，但是并没有完全实现调度器的功能
// 当前协程
namespace sylar
{
    Fiber *t_fiber = nullptr;
    // 主携程
    std::shared_ptr<Fiber> t_thread_fiber = nullptr;
    // 调度携程
    Fiber *t_scheduler_fiber = nullptr;

    std::atomic<uint64_t> s_fiber_id{0};
    std::atomic<uint64_t> s_fiber_count{0};
    // 1
    // 为啥这个不用makecontext
    // 创建的是主协程，而不是子协程
    Fiber::Fiber()
    {
        // this 指的是当前主协程的Fiber对象
        SetThis(this);
        m_state = RUNNING;
        getcontext(&m_ctx);
        s_fiber_count++;
        // 可以合并成一句m_id=s_fiber_id++;
        m_id = s_fiber_id;
        s_fiber_id++;
        if (debug)
            std::cout << "Fiber():main id = " << m_id << std::endl;
    }
    // 2
    // 创建字携程
    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler) : m_cb(cb), m_runInSheduler(run_in_scheduler)
    {
        m_state = READY;
        m_stacksize = stacksize ? stacksize : 128000;
        m_stack = malloc(m_stacksize);
       if(getcontext(&m_ctx)){
            std::cerr << "getcontext failed\n";
            pthread_exit(NULL);
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        // makecontext(&m_ctx,&MainFunc(),0);
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        s_fiber_count++;
        m_id = s_fiber_id;
        s_fiber_id++;
        if (debug)
            std::cout << "Fiber(): child id =" << m_id << std::endl;
    }
    Fiber::~Fiber()
    {
        s_fiber_count--;
        if (m_stack)
        {
            free(m_stack);
        }
        if (debug)
            std::cout << "~Fiber(): id = " << m_id << std::endl;
    }
    void Fiber::reset(std::function<void()> cb)
    {
        assert(m_stack != nullptr && m_state == TERM);
        // 重置说明他已经执行完毕了
        m_state = READY;
        m_cb = cb;
        // 问题在于 m_ctx 记录了旧的上下文状态，不重新初始化的话，协程可能仍然会执行旧的代码
        if (getcontext(&m_ctx))
        {
            std::cerr << "reset failed\n";
            pthread_exit(NULL);
        }
        // 这个是必须的，通过makecontext，m_ctx 被重置为调用MainFunc()，确保Fiber运行时会执行新的cb()
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    }
    // 恢复携程
    // 那恢复协程就只可能是从主协程或者是调度器恢复当前协程的执行
    void Fiber::resume()
    {
        assert(m_state == READY);
        m_state = RUNNING;
        if (m_runInSheduler)
        {
            std::cout << "1" << std::endl;
            // 这不是必须的，为了将局部的t_fiber这个表示当前正在与运行的实时更新
            SetThis(this);

            if (swapcontext(&t_scheduler_fiber->m_ctx, &m_ctx))
            {
                std::cerr << "resume() to t_scheduler_fiber failed\n";
                pthread_exit(NULL);
            }
        }
        else
        {
            std::cout << "2" << std::endl;
            SetThis(this);
            if (swapcontext(&t_thread_fiber->m_ctx, &m_ctx))
            {
                std::cerr << "resume() to t_thread_fiber failed\n";
                pthread_exit(NULL);
            }
        }
    }
    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }
    // 让出携程
    void Fiber::yeid()
    {
        assert(m_state == RUNNING || m_state == TERM);
        if (m_state != TERM)
        {
            m_state = READY;
        }
        if (m_runInSheduler)
        {
            SetThis(t_scheduler_fiber);
            if (swapcontext(&m_ctx, &t_scheduler_fiber->m_ctx))
            {
                std::cerr << "yield() to to t_scheduler_fiber failed\n";
                pthread_exit(NULL);
            }
        }
        else
        {
            // 这里需要用get()，因为t_thread_fiber是std::shared_ptr<Fiber>类型,要将他转化为裸指针
            SetThis(t_thread_fiber.get());

            if (swapcontext(&m_ctx, &t_thread_fiber->m_ctx))
            {
                std::cerr << "yield() to to t_thread_fiber failed\n";
                pthread_exit(NULL);
            }
        }
    }
    std::shared_ptr<Fiber> Fiber::GetThis()
    {
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        // 如果没有就创建主协程，并先将调度协程也设置成这样假装一下
        std::shared_ptr<Fiber> main_fiber(new Fiber());
        t_thread_fiber = main_fiber;
        t_scheduler_fiber = main_fiber.get();
        assert(t_fiber==main_fiber.get());
        // 这里难道不需要设置t_fiber=main_thread.get()吗？
        // 当然不需要呀，因为new Fiber()会调用无参数的Fiber，SetThis(this);他的这句是关键
        return t_fiber->shared_from_this();
    }
    uint64_t Fiber::GetFiberId()
    {
        if (t_fiber)
        {
            return t_fiber->m_id;
        }
        return -1;
    }
    // 在Fiber的实现中，MainFunc() 充当的是协程的真正入口函数，而m_cb则是用户提供的任务逻辑
    void Fiber::MainFunc()
    {
        std::shared_ptr<Fiber> curr = GetThis();
        curr->m_state = RUNNING;
        curr->m_cb();
        curr->m_cb = nullptr;
        curr->m_state = TERM;
        // 进行下一步
        auto raw_ptr = curr.get();
        // 释放 curr 持有的 shared_ptr，使其引用计数减一。
        // 因为 curr 是 shared_ptr<Fiber>，reset() 会让 curr 不再持有 Fiber 对象
        curr.reset();
        // 让出协程之前需要将
        raw_ptr->yeid();
        return;
    }
    // 调度器也是一个协程
    void Fiber::SetScheduler(Fiber *f)
    {
        t_scheduler_fiber = f;
    }
}
