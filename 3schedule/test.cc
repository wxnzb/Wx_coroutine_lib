// #include "schedule.h"
#include <iostream>
#include<unistd.h>
// using namespace sylar;
// std::mutex m; // 用于保护 std::cout 输出的互斥锁，因为 std::cout 是线程不安全的，需要通过锁来确保线程安全666
// static unsigned int i = 0;
// void task()
// {
//     std::lock_guard<std::mutex> lock(m);
//     std::cout << "task " << i++ << "is under prossing " << Thread::GetThreadId() << std::endl;
//     sleep(1);
//     return;
// }
// int main()
// {
//     // 创建调度器
//     std::shared_ptr<Scheduler> scheduler = std::make_shared<Scheduler>(3, true, "test");
//     scheduler->start();
//     sleep(2);
//     std::cout<<"\n post begin\n\n";
//     // 创建任务
//     for (int i = 0; i < 5; i++)
//     {
//         scheduler->scheduleLock(task);
//     }
//     sleep(3);
//     std::cout<<"\n post begin\n\n";
//     for (int i = 0; i < 10; i++)
//     {
//         scheduler->scheduleLock(task);
//     }
//     sleep(6);
//     scheduler->stop();
// }
#include "schedule.h"

using namespace sylar;

static unsigned int test_number;
std::mutex mutex_cout;
void task()
{
	{
		std::lock_guard<std::mutex> lock(mutex_cout);
		std::cout << "task " << test_number ++ << " is under processing in thread: " << Thread::GetThreadId() << std::endl;		
	}
	sleep(1);
}

int main(int argc, char const *argv[])
{
	{
		// 可以尝试把false 变为true 此时调度器所在线程也将加入工作线程
		std::shared_ptr<Scheduler> scheduler = std::make_shared<Scheduler>(3, true, "scheduler_1");
		
		scheduler->start();

		sleep(2);

		std::cout << "\nbegin post\n\n"; 
		for(int i=0;i<5;i++)
		{
			std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(task);
			scheduler->scheduleLock(fiber);
		}

		sleep(6);

		std::cout << "\npost again\n\n"; 
		for(int i=0;i<15;i++)
		{
			std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(task);
			scheduler->scheduleLock(fiber);
		}		

		sleep(3);
		// scheduler如果有设置将加入工作处理
		scheduler->stop();
	}
	return 0;
}