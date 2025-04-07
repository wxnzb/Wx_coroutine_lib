#ifndef _COROUTINE_H
#define _COROUTINE_H
#include<functional>
#include<ucontext.h>
#include<cstdint>
#include<memory>
#include<mutex>
namespace sylar{
class Fiber:public std::enable_shared_from_this<Fiber>{
    public:
        enum State{
            READY,
            RUNNING,
            TERM,
        };
        Fiber();
        Fiber(std::function<void()>cb,size_t stacksize=0,bool run_in_scheduler=true);
        ~Fiber();
        //重置协程
        void reset(std::function<void()>cb);
        //恢复协程
        void resume();
        //让出协程
        void yeid();
        uint64_t  getId(){return m_id;};//-----------------------------
        State getState(){return m_state;};
        public:
        //设置协程
        void SetThis(Fiber* f);
        //获取协程
        static std::shared_ptr<Fiber> GetThis();
        //获取协程id
        uint64_t GetFiberId();//----------------------
        //协程
        static void MainFunc();
        //设置调度协程，默认为主协程
        static void SetScheduler(Fiber* f);
        private:
        uint64_t m_id=0;
        State m_state=READY;
        std::function<void()>m_cb;
        size_t m_stacksize;
        bool m_runInSheduler;
        void* m_stack;//因为malloc返回的就是void*
        ucontext_t m_ctx;
        public:
        std::mutex m_mutex;
};
}
#endif