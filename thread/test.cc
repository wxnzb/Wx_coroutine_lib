#include<iostream>
#include<unistd.h>
#include<vector>
#include "thread.h"
using namespace sylar;
void func(){
    std::cout<<"id"<<Thread::GetThreadId()<<"name"<<Thread::GetName()<<std::endl;
    std::cout<<"this id"<<Thread::GetThis()->getId()<<"this name"<<Thread::GetThis()->getName()<<std::endl;
    sleep(60);
}
// int main(){
//     std::vector<Thread*> ths;
//     for(int i=0;i<10;i++){
//         Thread* th=new Thread(&func,"thread_"+std::to_string(i));
//         ths.push_back(th);
//     }
//     for(int i=0;i<10;i++){
//         ths[i]->join();
//     }
//     return 0;
// }
int main(){
    std::vector<std::shared_ptr<Thread>> ths;
    for(int i=0;i<10;i++){
        std::shared_ptr<Thread> th=std::make_shared<Thread>(&func,"thread_"+std::to_string(i));
        ths.push_back(th);
    }
    for(int i=0;i<10;i++){
        ths[i]->join();
    }
    return 0;
}