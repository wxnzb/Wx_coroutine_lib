#include<cstdint>
#include<functional>
#include<vector>
#include<memory>
#include<chrono>
#include<set>
#include<shared_mutex>
#include<mutex>
namespace sylar{
class Timer:public std::enable_shared_from_this<Timer>{
    friend class TimeManager;
    private:
    Timer(uint64_t ms,std::function<void()>cb,bool recurring,TimeManager* manager);
    uint64_t m_ms;
    std::function<void()>m_cb;
    TimeManager* m_manager;
    //是否循环
    bool m_recurring;
    //绝对超时时间
    std::chrono::time_point<std::chrono::steady_clock> m_next;
    
    public:
    //从时间堆删除Timer
    bool cancel();
    //刷新timer
    bool refresh();
    //重置timer的超时时间
    bool reset(uint64_t ms,bool from_now);
    //比较绝对超时时间函数
    struct Comparator{
        bool operator()(const std::shared_ptr<Timer>& a,const std::shared_ptr<Timer>& b)const;
    };
};
class TimeManager{
   friend class Timer;
   public:
   TimeManager();
   ~TimeManager();
   std::shared_ptr<Timer> addTimer(uint64_t ms,std::function<void()>cb,bool recurring);
   std::shared_ptr<Timer> addConditionTimer(uint64_t ms,std::function<void()> cb,bool curring,std::weak_ptr<void> weakcond);
   //堆中是否还有timer
   bool hasTimer();
   //取出所有时间的回调函数
   void listExpiredCb(std::vector<std::function<void()>>&cbs);
   //当有一个最早的timer加入堆中，调用它
   virtual void onTimerInsertedAtFront(){};
   //拿到堆中最近的超时时间
   uint64_t geteralistTime();
   protected:
   void addTimer(std::shared_ptr<Timer> timer);
   private:
   bool detectClockRollover();
   private:
   //时间堆
   std::set<std::shared_ptr<Timer>,Timer::Comparator> m_timers;
   //锁
   std::shared_mutex m_mutex;//读写锁
   bool m_tickle=false;
   std::chrono::time_point<std::chrono::steady_clock> m_previousTime;
   bool recurring=false;
};
}