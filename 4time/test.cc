#include "timer.h"
#include <iostream>
#include <unistd.h> //sleep需要
using namespace sylar;
void func(int i)
{
    std::cout << "i " << i << std::endl;
}
int main()
{
    // 先创建时间管理器
    std::shared_ptr<TimeManager> manager(new TimeManager());
    std::vector<std::function<void()>> cbs;
    // 测试listExpiredCb
    {
        for (int i = 0; i < 10; i++)
        {
            // 最后一个是是否循环
            manager->addTimer((i + 1) * 1000, std::bind(func, i), false);
        }
        sleep(5);
        manager->listExpiredCb(cbs);
        while (!cbs.empty())
        {
            std::function<void()> cb = *cbs.begin();
            cbs.erase(cbs.begin());
            cb();
        }
        sleep(5);
        manager->listExpiredCb(cbs);
        while (!cbs.empty())
        {
            std::function<void()> cb = *cbs.begin();
            cbs.erase(cbs.begin());
            cb();
        }
    }
    //测试recurring
    {
        manager->addTimer(1000,std::bind(func,1000),true);
        int j=10;
        while(j-->0){
            sleep(1);
            manager->listExpiredCb(cbs);
            std::function<void()> cb = *cbs.begin();
            cbs.erase(cbs.begin());
            cb();
        }
    }
    return 0;
}
