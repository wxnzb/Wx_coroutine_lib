#include<functional>
#include<ucontext.h>
#include<cstdint>
#include<memory>
class Fiber:public std::enable_shared_from_this<Fiber>{
    public:
    //锁
    std::mutex m_mutex;
        enum State{
            READY,
            RUNNING,
            TERM,
        };
        Fiber();
        Fiber(std::function<void()>cb,size_t stacksize,bool run_in_scheduler);
        ~Fiber();
        //重置携程
        void reset(std::function<void()>cb);
        //恢复携程
        void resume();
        //让出携程
        void yeid();
        uint64_t  getId(){return m_id;};//-----------------------------
        State getState(){return m_state;};
        public:
        //设置携程
        void SetThis(Fiber* f);
        //获取携程
        static std::shared_ptr<Fiber> GetThis();
        //获取携程id
        uint64_t GetFiberId();//----------------------
        //携程函数
        static void MainFunc();
        //设置调度携程，默认为主携程
        static void SetScheduler(Fiber* f);
        private:
        uint64_t m_id=0;
        State m_state=READY;
        std::function<void()>m_cb;
        size_t m_stacksize;
        bool m_runInSheduler;
        void* m_stack;//因为malloc返回的就是void*
        ucontext_t m_ctx;
};