#include <mutex>
#include <condition_variable>
#include <functional>
namespace sylar
{
    class Semaphore
    {
    public:
        // 值的注意，这里用了explicit，防止隐式转换，可以不这样做把
        //explicit Semaphore(int count_ = 0) : count(count_) {}
        Semaphore(int count_ = 0) : count(count_) {}
        void wait()
        {
            std::unique_lock<std::mutex> lock(mtx);
            while (count == 0)
            {
                cv.wait(lock);
            }
            count--;
        }
        void singal()
        {
            std::unique_lock<std::mutex> lock(mtx);
            count++;
            cv.notify_one();
        }

    private:
        int count = 0;
        std::mutex mtx;
        std::condition_variable cv;
    };
    class Thread
    {
    public:
        Thread(std::function<void()> cb, std::string name);
        ~Thread();
        void join();
        pid_t getId() const { return m_id; };
        const std::string &getName() { return m_name; };

    public:
        static Thread *GetThis();
        // 这里我感觉没有必要加static把
        // static void SetName(const std::string &name);
        void SetName(const std::string &name);
        static const std::string &GetName();
        static pid_t GetThreadId();

    private:
        static void *run(void *arg);

    private:
        std::function<void()> m_cb;
        std::string m_name;
        pid_t m_id = -1;
        Semaphore m_semaphore;
        // 要设置初值？？
        pthread_t m_thread = 0; // 关键
    };
}