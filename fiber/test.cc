#include "fiber.h"
#include<vector>
#include<iostream>
class Schedule{
    public:
    void schedule(std::shared_ptr<Fiber> task){
        m_tasks.push_back(task);
    }
    void run(){
        std::cout<<"run"<<m_tasks.size()<<std::endl;
        std::shared_ptr<Fiber>task;
        for(auto &it:m_tasks){
            task=it;
            task->resume();
        }
        m_tasks.clear();
    }
    private:
    std::vector<std::shared_ptr<Fiber>> m_tasks;
};
void A(int i){
    std::cout<<"Hello,wx,+"<<i<<std::endl;
}
int main(){
     Fiber::GetThis();
     Schedule sc;
     for(int i=0;i<10;i++){
        std::shared_ptr<Fiber> task=std::make_shared<Fiber>(std::bind(A,i),false,0);
        sc.schedule(task);
     }
     sc.run();
}