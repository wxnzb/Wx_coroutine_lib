#include <cstdint>
#include<memory>
#include<vector>
class FdCtx
{
public:
    FdCtx(int fd);
    ~FdCtx();
    bool init();
    bool isInit() { return m_isInit; };
    bool isSocket() { return m_isSocket; };
    bool isClose() { return m_isClose; };

    void setUserNonblock(bool v) { m_userNonblock = v; };
    bool getUserNonblock() { return m_userNonblock; };

    void setSysNonblock(bool v) { m_sysNonblock = v; };
    bool getSysNonblock() { return m_sysNonblock; };
    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

private:
    int m_fd;
    bool m_isInit;
    bool m_isSocket;
    bool m_isClose;
    bool m_userNonblock;
    bool m_sysNonblock;
    uint64_t m_recvTimeout = (uint64_t)-1;
    uint64_t m_sendTimeout = (uint64_t)-1;
};
class FdManager{
    public:
    FdManager();
    std::shared_ptr<FdCtx> add(int fd,bool auto_create =false);
    void del(int fd);
    private:
    std::shared_mutex m_mutex;
    std::vector<std::shared_ptr<FdCtx>> m_datas;
};
template <typename T>
class Singleton
{
public:
     //禁止拷贝构造和赋值
     Singleton(const Singleton &)=delete;
     Singleton &operator=(const Singleton &)=delete;
    static T *getInstanse()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (instance == nullptr)
        {
            instance = new T();
        }
        return instance;
    }
    static void delInstance()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (instance)
        {
            delete instance;
            instance = nullptr;
        }
    }
   protected:
   Singleton(){};
private:
    static T *instance;
    std::mutex m_mutex;
};