#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <vector>
#include "thread.h"
#include "fiber.h"
namespace sylar{
class Scheduler{
    public:
        Scheduler(size_t threads=1,bool user_caller=true,const std::string & m_name="Scheduler");
        ~Scheduler();
        const std::string& getName(){return m_name;};
    public:
    static Scheduler* GetThis();
    void SetThis();
    void start();
    void stop();
    void run();
    template<class FiberOrcb>
    //这里判断有没有任务，没有的话，就是true，然后你将要你要执行的携程或者回调函数加入队列，在通过tickle()运行
    void scheduleLock(FiberOrcb fc,int thread=-1){
          bool need_tickle;
          {
            //这里为什么要加锁,防止同时修改m_tasks
            std::lock_guard<std::mutex>lock(m_mutex);
            need_tickle=m_tasks.empty();
            ScheduleTask task(fc,thread);
            if(task.fiber||task.cb){
                m_tasks.push_back(task);
            }
          }
          if(need_tickle){
            tickle();
          }
    }
    virtual void tickle();
    //是否可以关闭
    virtual bool stopping();
    //空闲携程函数
    virtual void idle();
    bool hasIdleThreads(){return m_idleThreadCount>0;}
    public:
    struct ScheduleTask{
        std::shared_ptr<Fiber> fiber;
        std::function<void()>cb;
        int thread_id;
        ScheduleTask(std::shared_ptr<Fiber>f,int thread){
            fiber=f;
            thread_id=thread;
        }
        ScheduleTask(std::shared_ptr<Fiber>*f,int thread){
            fiber.swap(*f);
            thread_id=thread;
        }
        ScheduleTask(std::function<void()>f,int thread){
            cb=f;
            thread_id=thread;
        }
        ScheduleTask(std::function<void()>*f,int thread){
            cb.swap(*f);
            thread_id=thread;
        }
        ScheduleTask(){
            fiber=nullptr;
            cb=nullptr;
            thread_id=-1;
        }
        void reset(){
            fiber=nullptr;
            cb=nullptr;
            thread_id=-1;
        }
    };
    private:
    bool m_user_caller;
    int m_user_id;
    //是否正在关闭
    bool m_stopping=false;
    std::string m_name;
    //调度携程
    std::shared_ptr<Fiber>m_schedulerFiber;
    //主线程id
    int m_rootId=-1;
    //工作线程的线程id
    std::vector<int> m_threadIds;
    //额外需要创建的线程数
    size_t m_threadNum;
    std::vector<std::shared_ptr<Thread>> m_threads;
    //任务队列
    std::vector<ScheduleTask> m_tasks;
    //活跃线程数量
    int m_activeThreadCount;
    //空闲线程数量
    int m_idleThreadCount;
    //环境变量
    std::condition_variable m_cond;
    //互斥变量
    std::mutex m_mutex;
};
}
#endif 