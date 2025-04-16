#include "iomanager.h"
#include <sys/epoll.h>
#include <unistd.h> //pipe
#include <fcntl.h>
#include <iostream>
static bool debug = true;
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
        stop();
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
        throw std::invalid_argument("Unsupported event type");
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
        return;
    }
    void IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;
        std::shared_lock<std::shared_mutex> read_lock(m_mutex);
        if (fd > int(m_fdContexts.size()))
        {
            fd_ctx = m_fdContexts[fd];
            read_lock.unlock();
        }
        else
        {
            read_lock.unlock();
            std::unique_lock<std::shared_mutex> write_lock(m_mutex);
            contextSize(1.5 * fd);
            fd_ctx = m_fdContexts[fd];
        }
        std::lock_guard<std::mutex> lock(fd_ctx->m_mutex);
        if (fd_ctx->events & event)
        {
            return;
        }
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event ev;
        ev.events = EPOLLET | event | fd_ctx->events;
        ev.data.ptr = fd_ctx;
        epoll_ctl(m_epollfd, op, fd, &ev);
        m_pendingEventCount++;
        fd_ctx->events = Event(fd_ctx->events | event);
        FdContext::EventContext &ctx = fd_ctx->getEventContext(event);
        ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            ctx.cb.swap(cb);
        }
        else
        {
            ctx.fiber = Fiber::GetThis();
        }
    }
    void IOManager::delEvent(int fd, Event event)
    {
        FdContext *fd_ctx = nullptr;
        std::shared_lock<std::shared_mutex> read_lock(m_mutex);
        if (fd > int(m_fdContexts.size()))
        {
            fd_ctx = m_fdContexts[fd];
            read_lock.unlock();
        }
        else
        {
            read_lock.unlock();
            return;
        }
        std::lock_guard<std::mutex> lock(fd_ctx->m_mutex);
        int op = (fd_ctx->events & ~event) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ev;
        ev.events = EPOLLET | (fd_ctx->events & ~event);
        ev.data.ptr = fd_ctx;
        epoll_ctl(m_epollfd, op, fd, &ev);
        m_pendingEventCount--;
        fd_ctx->events = Event(fd_ctx->events & ~event);
        FdContext::EventContext &ctx = fd_ctx->getEventContext(event);
        fd_ctx->resetEventContext(ctx);
    }
    void IOManager::cancelEvent(int fd, Event event)
    {
        FdContext *fd_ctx = nullptr;
        std::shared_lock<std::shared_mutex> read_lock(m_mutex);
        if (fd > int(m_fdContexts.size()))
        {
            fd_ctx = m_fdContexts[fd];
            read_lock.unlock();
        }
        else
        {
            read_lock.unlock();
            return;
        }
        std::lock_guard<std::mutex> lock(fd_ctx->m_mutex);
        int op = (fd_ctx->events & ~event) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ev;
        ev.events = EPOLLET | (fd_ctx->events & ~event);
        ev.data.ptr = fd_ctx;
        epoll_ctl(m_epollfd, op, fd, &ev);
        m_pendingEventCount--;
        fd_ctx->triggerEvent(event);
        return;
    }
    void IOManager::cancelAll(int fd)
    {
        FdContext *fd_ctx = nullptr;
        std::shared_lock<std::shared_mutex> read_lock(m_mutex);
        if (fd > int(m_fdContexts.size()))
        {
            fd_ctx = m_fdContexts[fd];
            read_lock.unlock();
        }
        else
        {
            read_lock.unlock();
            return;
        }
        if (fd_ctx->events == NONE)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(fd_ctx->m_mutex);
        int op = EPOLL_CTL_DEL;
        epoll_event ev;
        ev.events = 0;
        ev.data.ptr = fd_ctx;
        epoll_ctl(m_epollfd, op, fd, &ev);
        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            m_pendingEventCount--;
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            m_pendingEventCount--;
        }
        return;
    }
    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }
    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            return;
        }
        write(m_tickleFds[0], "T", 1);
    }
    void IOManager::idle()
    {
        static const uint64_t MAX_EVENTS = 256;
        std::unique_ptr<epoll_event[]> events(new epoll_event[MAX_EVENTS]);
        while (true)
        {
            if (debug)
                std::cout << "IOManager::idle(),run in thread: " << Thread::GetThreadId() << std::endl;
            if (stopping())
            {
                if (debug)
                    std::cout << "name = " << getName() << " idle exits in thred: " << Thread::GetThreadId() << std::endl;
                break;
            }
            int n;
            while (true)
            {
                uint64_t MAX_TIMEOUT = 5000;
                uint64_t timeout = geteralistTime();
                uint64_t min = std::min(timeout, MAX_TIMEOUT);
                n = epoll_wait(m_epollfd, events.get(), MAX_EVENTS, min);
            }
            // 将超时任务加入队列
            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty())
            {
                std::cout<<"十分怀疑没进去"<<std::endl;
                for (auto &cb : cbs)
                {
                    scheduleLock(cb);
                }
                cbs.clear();
            }
            for (int i = 0; i < n; i++)
            {
                epoll_event &ev = events[i];
                if(ev.data.fd==m_tickleFds[0]){
                    uint8_t dummy[256];
                    while(read(m_tickleFds[0],dummy,sizeof(dummy))>0);
                    continue;
                }
                FdContext *fd_ctx = (FdContext *)ev.data.ptr;
                if (ev.events & (EPOLLERR | EPOLLHUP))
                {
                    ev.events = ev.events & (EPOLLIN & EPOLLOUT);
                }
                int event = NONE;
                if (ev.events & EPOLLIN)
                {
                    event |= READ;
                }
                if (ev.events & EPOLLOUT)
                {
                    event |= WRITE;
                }
                std::lock_guard<std::mutex> lock(fd_ctx->m_mutex);
                int op = (ev.events & ~event) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                ev.events = EPOLLET | (ev.events * event);
                epoll_ctl(m_epollfd, op, fd_ctx->fd, &ev);
                if (event & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    m_pendingEventCount--;
                }
                if (event & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    m_pendingEventCount--;
                }
            }
            Fiber::GetThis()->yeid();
        }
    }
    bool IOManager::stopping()
    {
        uint64_t timeout = geteralistTime();
        return Scheduler::stopping() && timeout == ~0ull && m_pendingEventCount == 0;
    }
    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }
}