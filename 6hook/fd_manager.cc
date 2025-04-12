#include "fd_manager.h"
#include<sys/stat.h>
#include<fcntl.h>
#include<mutex>
namespace sylar{
    template class Singleton<FdManager>;
    template <typename T>
    T* Singleton<T>::instance=nullptr;
    template <typename T>
    std::mutex Singleton<T>::m_mutex;
FdCtx::FdCtx(int fd){
    m_fd=fd;
    init();
}
FdCtx::~FdCtx(){

}
bool FdCtx::init(){
   //获取文件描述符的状态
   struct stat st;
   if(fstat(m_fd,&st)==-1){
    m_isInit=false;
    m_isSocket=false;//这个为什么要设置
   }else{
    m_isInit=true;
    m_isSocket=S_ISSOCK(st.st_mode);
   }
   //如果文件描述符是套接字
   if(m_isSocket){
    //看这个套子节是否是非阻塞的
    int flags=fcntl(m_fd,F_GETFL,0);
    //如果文件描述符不是非阻塞的
    if(!(flags)&O_NONBLOCK){
        //设置文件描述符为非阻塞
        fcntl(m_fd,F_SETFD,flags|O_NONBLOCK);
        m_sysNonblock=true;
    }else{
        m_sysNonblock=false;
    }
   }else{
    m_sysNonblock=false;
}
   return m_isInit;
}
void FdCtx::setTimeout(int type,uint64_t v){
    if(type==SO_RECVTIMEO){
        m_recvTimeout=v;
    }else{
        m_sendTimeout=v;
    }
}
uint64_t FdCtx::getTimeout(int type){
    if(type==SO_RECVTIMEO){
        return m_recvTimeout;
    }
    return m_sendTimeout;
}
FdManager::FdManager(){
    m_datas.resize(64);
}
std::shared_ptr<FdCtx> FdManager::get(int fd,bool auto_create){
    if(fd==-1){
        return nullptr;
    }
    std::shared_lock<std::shared_mutex>read_lock(m_mutex);
    if(fd>=m_datas.size()){
        if(auto_create==false){
            return nullptr;
        }
        else{
            read_lock.unlock();
            std::unique_lock<std::shared_lock>write_lock(m_mutex);
            m_datas.resize(fd*1.5);
            m_datas[fd]=std::shared_ptr<FdCtx>(new FdCtx(fd));
            return m_datas[fd];
        }
    }
    return m_datas[fd];
}
void FdManager::del(int fd){
    std::shared_lock<std::shared_mutex>write_lock(m_mutex);
    if(fd>=m_datas.size()){
        return;
    }
    m_datas[fd]=nullptr;
}
}