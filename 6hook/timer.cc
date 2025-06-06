// #include "timer.h"
// namespace sylar{
// Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimeManager *manager) : m_ms(ms), m_cb(cb), m_recurring(recurring), m_manager(manager)
// {
//    auto now = std::chrono::steady_clock::now();
//    m_next = now + std::chrono::milliseconds(m_ms);
// }
// bool Timer::Comparator::operator()(const std::shared_ptr<Timer> &a, const std::shared_ptr<Timer> &b) const
// {
//    return a->m_next < b->m_next;
// }
// // 把自己这个定时器从管理中删除
// bool Timer::cancel()
// {
//    // 首先要记得加锁
//    std::unique_lock<std::shared_mutex> write_lock(m_manager->m_mutex);
//    if (m_cb == nullptr)
//    {
//       return false;
//    }
//    m_cb = nullptr;
//    auto it = m_manager->m_timers.find(shared_from_this());
//    if (it != m_manager->m_timers.end())
//    {
//       m_manager->m_timers.erase(it);
//    }
//    return true;
// }
// bool Timer::refresh()
// {
//    // 这个和上面简直一模一样，感觉可以这样写cancle()
//    std::unique_lock<std::shared_mutex> write_lock(m_manager->m_mutex);
//    if (m_cb == nullptr)
//    {
//       return false;
//    }
//    auto it = m_manager->m_timers.find(shared_from_this());
//    if (it == m_manager->m_timers.end())
//    {
//       return false;
//    }
//    m_manager->m_timers.erase(it);
//    // 先删除并重新设置并插入
//    m_next = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_ms);
//    m_manager->m_timers.insert(shared_from_this());
// }
// // 这个和上面那个不同，主要是是上面那个可定是往后的，但是下面这个可能在定时器最前面
// bool Timer::reset(uint64_t ms, bool from_now)
// {
//    if (m_ms == ms)
//    {
//       return true;
//    }
//    {
//       std::unique_lock<std::shared_mutex> write_lock(m_manager->m_mutex);
//       if (m_cb == nullptr)
//       {
//          return false;
//       }
//       auto it = m_manager->m_timers.find(shared_from_this());
//       if (it == m_manager->m_timers.end())
//       {
//          return false;
//       }
//       m_manager->m_timers.erase(it);
//    }
//    auto start = from_now ? std::chrono::steady_clock::now() : m_next - std::chrono::milliseconds(m_ms);
//    m_ms = ms;
//    m_next = start + std::chrono::milliseconds(m_ms);
//    // m_manager->m_timers.insert(shared_from_this());
//    m_manager->addTimer(shared_from_this());
// }
// TimeManager::TimeManager()
// {
//    m_previousTime = std::chrono::steady_clock::now();
// }
// TimeManager::~TimeManager()
// {
// }
// void TimeManager::addTimer(std::shared_ptr<Timer> timer)
// {
//    bool at_front = false;
//    {
//       std::unique_lock<std::shared_mutex> write_lock(m_manager->mutex);
//       auto it = m_timers.insert(timer).first;
//       at_front = (it == m_manager->m_timers.begin()) && !m_tickle;
//       if (at_front)
//       {
//          m_tickle = true;
//       }
//    }
//    if (at_front)
//    {
//       // 叫醒
//       onTimeInsertedAtFront();
//    }
// }
// std::shared_ptr<Timer> TimeManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
// {
//    std::shared_ptr<Timer> timer(new Timer(ms, cb, recurring, this));
//    addTimer(timer);
//    return timer;
// }
// void OnTimer(std::weak_ptr<void> weakcond, std::function<void()> cb)
// {
//    std::shared_ptr<void> cond = weakcond.lock();
//    if (cond)
//    {
//       cb();
//    }
// }
// std::shared_ptr<Timer> TimeManager::addConditionTimer(uint64_t ms, std::function<void()> cb, bool recurring, std::weak_ptr<void> weakcond)
// {
//    return addTimer(ms, std::bind(&OnTimer, weakcond, cb), recurring);
// }
// bool TimeManager::hasTimer()
// {
//    std::unique_lock<std::shared_mutex> write_lock(m_manager->mutex);
//    return !m_timers.empty();
// }
// void TimeManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
// {
//    auto now = std::chrono::steady_clock::now();
//    bool rollover = detectClockRollover();
//    std::unique_lock<std::shared_mutex> write_lock(m_mutex);
//    while ((!m_timers.empty() && rollover) || ((!m_timers.empty() && (*m_timers.begin())->m_next < now)))
//    {
//       auto timer = *m_timers.begin();
//       m_timers.erase(m_timers.begin());
//       if (timer->m_cb)
//       {
//          cbs.push_back(timer->m_cb);
//       }
//       if (recurring)
//       {
//          // auto now=std::chrono::steady_clock::now();直接用上面那个就行吗，不会存在时差吗
//          timer->m_next = now + std::chrono::milliseconds(timer->m_ms);
//          m_timers.insert(timer);
//       }
//       else
//       {
//          timer->m_cb = nullptr;
//       }
//    }
// }
// // 检测系统时钟是否发生回绕（Clock Rollover）
// bool TimeManager::detectClockRollover()
// {
//    bool rollover = false;
//    auto now = std::chrono::steady_clock::now();
//    if (now < m_previousTime - std::chrono::milliseconds(60 * 60 * 1000))
//    {
//       rollover = true;
//    }
//    m_previousTime = now;
//    return rollover;
// }
// // 拿到堆中最近的超时时间
// uint64_t TimeManager::geteralistTime()
// {
//    std::unique_lock<std::shared_mutex> read_lock(m_mutex);
//    // 为什么要加这句??
//    m_tickle = false;
//    if (m_timers.empty())
//    {
//       return ~0ull;
//    }
//    auto time = (*m_timers.begin())->m_next;
//    auto now = std::chrono::steady_clock::now();
//    if (time < now)
//    {
//       return 0;
//    }
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(time - now).count();
//    return static_cast<uint64_t>(duration);
// }
// }
#include "timer.h"
namespace sylar
{
   Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimeManager *manager) : m_ms(ms), m_cb(cb), m_recurring(recurring), m_manager(manager)
   {
      auto now = std::chrono::steady_clock::now();
      m_next = now + std::chrono::milliseconds(m_ms);
   }
   bool Timer::Comparator::operator()(const std::shared_ptr<Timer> &a, const std::shared_ptr<Timer> &b) const
   {
      return a->m_next < b->m_next;
   }
   // 把自己这个定时器从管理中删除
   bool Timer::cancel()
   {
      // 首先要记得加锁
      std::unique_lock<std::shared_mutex> write_lock(m_manager->m_mutex);
      if (m_cb == nullptr)
      {
         return false;
      }
      m_cb = nullptr;
      auto it = m_manager->m_timers.find(shared_from_this());
      if (it != m_manager->m_timers.end())
      {
         m_manager->m_timers.erase(it);
      }
      return true;
   }
   bool Timer::refresh()
   {
      // 这个和上面简直一模一样，感觉可以这样写cancle()
      std::unique_lock<std::shared_mutex> write_lock(m_manager->m_mutex);
      if (m_cb == nullptr)
      {
         return false;
      }
      auto it = m_manager->m_timers.find(shared_from_this());
      if (it == m_manager->m_timers.end())
      {
         return false;
      }
      m_manager->m_timers.erase(it);
      // 先删除并重新设置并插入
      m_next = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_ms);
      m_manager->m_timers.insert(shared_from_this());
      return true;
   }
   // 这个和上面那个不同，主要是是上面那个可定是往后的，但是下面这个可能在定时器最前面
   bool Timer::reset(uint64_t ms, bool from_now)
   {
      // 这里要记得加上from_now
      if (m_ms == ms && !from_now)
      {
         return true;
      }
      {
         std::unique_lock<std::shared_mutex> write_lock(m_manager->m_mutex);
         if (m_cb == nullptr)
         {
            return false;
         }
         auto it = m_manager->m_timers.find(shared_from_this());
         if (it == m_manager->m_timers.end())
         {
            return false;
         }
         m_manager->m_timers.erase(it);
      }
      auto start = from_now ? std::chrono::steady_clock::now() : m_next - std::chrono::milliseconds(m_ms);
      m_ms = ms;
      m_next = start + std::chrono::milliseconds(m_ms);
      // m_manager->m_timers.insert(shared_from_this());
      m_manager->addTimer(shared_from_this());
      return true;
   }
   TimeManager::TimeManager()
   {
      m_previousTime = std::chrono::steady_clock::now();
   }
   TimeManager::~TimeManager()
   {
   }
   void TimeManager::addTimer(std::shared_ptr<Timer> timer)
   {
      bool at_front = false;
      {
         std::unique_lock<std::shared_mutex> write_lock(m_mutex);
         auto it = m_timers.insert(timer).first;
         at_front = (it == m_timers.begin()) && !m_tickle;
         if (at_front)
         {
            m_tickle = true;
         }
      }
      if (at_front)
      {
         // 叫醒
         onTimerInsertedAtFront();
      }
   }
   std::shared_ptr<Timer> TimeManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
   {
      std::shared_ptr<Timer> timer(new Timer(ms, cb, recurring, this));
      addTimer(timer);
      return timer;
   }
   void OnTimer(std::weak_ptr<void> weakcond, std::function<void()> cb)
   {
      std::shared_ptr<void> cond = weakcond.lock();
      if (cond)
      {
         cb();
      }
   }
   std::shared_ptr<Timer> TimeManager::addConditionTimer(uint64_t ms, std::function<void()> cb,  std::weak_ptr<void> weakcond,bool recurring)
   {
      return addTimer(ms, std::bind(&OnTimer, weakcond, cb), recurring);
   }
   bool TimeManager::hasTimer()
   {
      std::unique_lock<std::shared_mutex> write_lock(m_mutex);
      return !m_timers.empty();
   }
   void TimeManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
   {
      auto now = std::chrono::steady_clock::now();
      std::unique_lock<std::shared_mutex> write_lock(m_mutex);
      bool rollover = detectClockRollover();
      while ((!m_timers.empty() && rollover) || ((!m_timers.empty() && (*m_timers.begin())->m_next < now)))
      {
         std::shared_ptr<Timer> timer = *m_timers.begin();
         m_timers.erase(m_timers.begin());
         if (timer->m_cb)
         {
            cbs.push_back(timer->m_cb);
         }
         if (timer->m_recurring)
         {
            // auto now=std::chrono::steady_clock::now();直接用上面那个就行吗，不会存在时差吗
            timer->m_next = now + std::chrono::milliseconds(timer->m_ms);
            m_timers.insert(timer);
         }
         else
         {
            timer->m_cb = nullptr;
         }
      }
   }
   // 检测系统时钟是否发生回绕（Clock Rollover）
   bool TimeManager::detectClockRollover()
   {
      bool rollover = false;
      auto now = std::chrono::steady_clock::now();
      if (now < m_previousTime - std::chrono::milliseconds(60 * 60 * 1000))
      {
         rollover = true;
      }
      m_previousTime = now;
      return rollover;
   }
   // 拿到堆中最近的超时时间
   uint64_t TimeManager::geteralistTime()
   {
      std::unique_lock<std::shared_mutex> read_lock(m_mutex);
      // 为什么要加这句??
      m_tickle = false;
      if (m_timers.empty())
      {
         return ~0ull;
      }
      auto now = std::chrono::steady_clock::now();
      auto time = (*m_timers.begin())->m_next;
      if (time <= now)
      {
         return 0;
      }
      else
      {
         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(time - now).count();
         return static_cast<uint64_t>(duration);
      }
   }
}
