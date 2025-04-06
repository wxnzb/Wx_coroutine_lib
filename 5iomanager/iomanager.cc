#include "iomanager.h"
#include <sys/epoll.h>
#include <unistd.h> //pipe
#include <fcntl.h>
namespace sylar
{
    IOManager::IOManager(size_t threads, bool user_caller, const std::string &name) : Scheduler(threads, user_caller, name), TimeManager()
    {
        m_epollfd = epoll_create(5000);
        pipe(m_tickleFds);
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = m_tickleFds[0];
        // 设置非阻塞
        fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_tickleFds[0], &ev);
        contextSize(32);
        start();
    }
    void IOManager::contextSize(size_t size)
    {
        m_fdContexts.resize(size);
        for (int i = 0; i < size; i++)
        {
            if (m_fdContexts[i] == nullptr)
            {
                m_fdContexts[i] = new FdContext();
                m_fdContexts[i]->fd = i;
            }
        }
    }
    IOManager::~IOManager()
    {
        close(m_epollfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);
        for (int i = 0; i < m_fdContexts.size(); i++)
        {
            delete m_fdContexts[i];
        }
    }
    IOManager::FdContext::EventContext &IOManager::FdContext::getEventContext(IOManager::Event event)
    {
        switch (event)
        {
        case READ:
            return read;
        case WRITE:
            return write;
        }
    }
    void IOManager::FdContext::resetEventContext(IOManager::FdContext::EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        // 为什么不这样写ctx.fiber = nullptr;
        ctx.cb = nullptr;
    }
    void IOManager::FdContext::triggerEvent(IOManager::Event event)
    {
        events = Event(events & ~event);
        EventContext &ctx = getEventContext(event);
        if (ctx.fiber)
        {
            ctx.scheduler->scheduleLock(ctx.fiber);
        }
        if (ctx.cb)
        {
            ctx.scheduler->scheduleLock(ctx.cb);
        }
        resetEventContext(ctx);
    }
    void IOManager::addEvent(int fd, Event event, std::function<void()> cb){
        FdContext* fd_ctx=nullptr;
        if(fd>=int(m_fdContexts.size())){
            fd_ctx=m_fdContexts[fd];
        }else{
            contextSize(1.5*fd);
            fd_ctx=m_fdContexts[fd];
        }
        if(fd_ctx->events&event){
            return;
        }
        int op=fd_ctx->events?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
        epoll_event  ev;
        ev.events=EPOLLET|event|fd_ctx->events;
        ev.data.ptr=fd_ctx;
        epoll_ctl(m_epollfd,op,fd,&ev);
        m_pendingEventCount++;
        fd_ctx->events=Event(fd_ctx->events|event);
        FdContext::EventContext& ctx=fd_ctx->getEventContext(event);
        ctx.scheduler=Scheduler::GetThis();
        if(cb){
            ctx.cb.swap(cb);
        }else{
            ctx.fiber=Fiber::GetThis();
        }
    }
    void IOManager::delEvent(int fd,Event event){
        FdContext* fd_ctx=nullptr;
        if(fd>=int(m_fdContexts.size())){
            fd_ctx=m_fdContexts[fd];
        }else{
            return;
        }
        int op=(fd_ctx->events&~event)?EPOLL_CTL_MOD:EPOLL_CTL_DEL;
        epoll_event ev;
        ev.events=EPOLLET|(fd_ctx->events&~event);
        ev.data.ptr=fd_ctx;
        epoll_ctl(m_epollfd,op,fd,&ev);
        m_pendingEventCount--;
        
    }
}
