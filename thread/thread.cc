#include "thread.h"
#include<iostream>
#include<sys/syscall.h>
#include<unistd.h>
namespace sylar{
static thread_local Thread* t_thread=nullptr;
static thread_local std::string t_thread_name="UNKOWN";
Thread::Thread(std::function<void()> cb,std::string name):m_name(name),m_cb(cb){
    int rt=pthread_create(&m_thread,nullptr,&Thread::run,this);
    if(rt){
        std::cout<<"pthread_create error"<<std::endl;
    }
    m_semaphore.wait();
}
Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
        m_thread=0;
    }
}
void Thread::join(){
    if(m_thread){
        int rt=pthread_join(m_thread,nullptr);
        if(rt){
            std::cout<<"pthread_join error"<<std::endl;
        }
        m_thread=0;
    }
}
void* Thread::run(void* arg){
    //这里必须重新设置一个，不能用t_thread这个全局变量
    Thread* thread=(Thread*)arg;
     thread->m_id=GetThreadId();

     t_thread=thread;
     t_thread_name=thread->m_name;

     pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());
     //回调
    // t_thread->m_cb();这样好像可以提高效率
    std::function<void()>cb;
    cb.swap(thread->m_cb);
    thread->m_semaphore.singal();
    cb();
    return 0;
    // Thread* thread = (Thread*)arg;

    // t_thread       = thread;
    // t_thread_name  = thread->m_name;
    // thread->m_id   = GetThreadId();
    // pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    // std::function<void()> cb;
    // cb.swap(thread->m_cb); // swap -> 可以减少m_cb中只能指针的引用计数
    
    // // 初始化完成
    // thread->m_semaphore.singal();

    // cb();
    // return 0;
}
pid_t Thread::GetThreadId(){
    return syscall(SYS_gettid);
}
void Thread::SetName(const std::string &name){
    if(t_thread){
    t_thread->m_name=name;
    }
    t_thread_name=name;
}
Thread* Thread::GetThis(){
    return t_thread;
}
const std::string& Thread::GetName(){
   // return t_thread->m_name;
   return t_thread_name;
}
}