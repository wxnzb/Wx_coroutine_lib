#include <vector>
#include "thread.h"
#include "fiber.h"
class Scheduler{
    public:
        Scheduler();
        ~Scheduler();
        const std::string& getName(){return m_name;};
    public:
    Scheduler* GetThis();
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
            tickle()
          }
    }
    void tickle();
    //是否可以关闭
    bool stoping();
    //空闲携程函数
    void idle();
    private:
    std::string m_name;
    std::mutex m_mutex;
    std::vector<std::shared_ptr<Fiber>> m_tasks;
    public:
    struct ScheduleTask{
        std::shared_ptr<Fiber> fiber;
        std::function<void()>cb;
        int thread;
        ScheduleTask(std::shared_ptr<Fiber>f,int thread){
            fiber=f;
            thread=thread;
        }
        ScheduleTask(std::shared_ptr<Fiber>*f,int thread){
            fiber.swap(*f);
            thread=thread;
        }
        ScheduleTask(std::function<void()>f,int thread){
            cb=f;
            thread=thread;
        }
        ScheduleTask(std::function<void()>*f,int thread){
            cb.swap(*f);
            thread=thread;
        }
        ScheduleTask(){
            fiber=nullptr;
            cb=nullptr;
            thread=-1;
        }
        void reset(){
            fiber=nullptr;
            cb=nullptr;
            thread=-1;
        }
    };
};