#include "schedule.h"
#include "timer.h"
namespace sylar
{
    class IOManager : public Scheduler,public TimeManager
    {
    public:
        enum Event
        {
            NONE = 0x0,
            READ = 0x1,
            WRITE = 0x2,
        };

    private:
        struct FdContext
        {
            struct EventContext
            {
                Scheduler *scheduler = nullptr; // Scheduler他是一个类，可以直接用是因为继承
                std::shared_ptr<Fiber> fiber;
                std::function<void()> cb;
            };
            int fd = 0;
            EventContext read;
            EventContext write;
            Event events = NONE;
            EventContext &getEventContext(Event event);
            // 清空事件
            void resetEventContext(EventContext &ctx); // 不用Event event
            // 触发事件并执行回调
            void triggerEvent(Event event);
        };

    public:
        IOManager(size_t threads=1, bool user_caller=true,const std::string& name = "IOManager");
        ~IOManager();
        void addEvent(int fd, Event event, std::function<void()> cb);
        void delEvent(int fd, Event event);
        void cancelEvent(int fd, Event event);
        void cancelAll(int fd);

    protected:
       //这是schedule.h里面的
        void tickle() override;
        // 是否可以关闭
        bool stopping() override;
        // 空闲携程函数
        void idle() override;
        //这个是timer.h里面的
        void onTimerInsertedAtFront() override;
        //设置文件描述符多少
        void contextSize(size_t size);
    private:
        int m_epollfd;
        int m_tickleFds[2];
        std::vector<FdContext *> m_fdContexts;
        int m_pendingEventCount=0;
    };

}