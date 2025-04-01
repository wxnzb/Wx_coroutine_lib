## 1
- 为什么 run() 不能直接访问 m_cb，但 Thread::Thread() 里可以访问 m_semaphore
```
✅ 因为 run() 是 static，它没有 this，无法直接访问 m_cb。

✅ 但 Thread::Thread() 是构造函数，它有 this，所以可以访问 m_semaphore。

✅ 在 run() 里，我们必须用 static_cast<Thread*>(arg) 来恢复 this，才能访问 m_cb。

```
## 2
- m_semaphore.wait();这一行的作用是什么？
- 他就是使得创建的这个Thread类阻塞，等待完成一些操作比如等待子线程的初始化或某些关键操作完成。通常情况下，子线程会在某些初始化后调用 m_semaphore.signal()，从而唤醒这个Thread类

## 3
- pthread_detach(m_thread)                       VS                              pthread_join(m_thread, nullptr);
- 将一个线程标记为分离状态，意味着该线程的资源会在线程结束时自动回收，而不需要其他线程调用 pthread_join 来回收它的资源，适用于不想知道这个线程运行的结果
- 等待指定线程执行完毕，并回收与该线程相关的资源。
## 4
- pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
- 不是在Thread初始化的时候已经设置了m_name了吗，这里为什么还需要在此设置？
- Thread 构造函数中初始化了线程名称（m_name），但线程的名称实际设置是在线程启动后的执行函数中进行的。

## 5
```
std::function<void()> cb;
        cb.swap(thread->m_cb);
        cb();
        //t_thread->m_cb();//上面这样好像可以提高效率
```
- 为什么不用下面这个
- 为了提高效率、避免潜在的线程安全问题和确保回调的正确执行顺序

## 6
- run中的thread->m_semaphore.singal();是怎么和Thread中的 m_semaphore.wait();相呼应的，Thread中的本来通过他阻塞了，然后single唤醒了，那么run中的cb()是怎么执行的？
- 执行过程:
- 创建Thread类，在这个类里面创建了自线程并开始在自线程里面执行run,现在Thread这里是通过wait阻塞的，这里阻塞的目的是让在子线程里面完成有关Thread的相关初始化，然后run里面完成后通过single通知Thread类，现在就是两个线程一起在执行

## 7
- Thread类他是在主线程Main()函数中创建并执行的

## 8
- test.cc中的主要执行流程
- 首先for循环中创建了10个Thread类，先完成名字的初始化，在每一个类中都创建了子线程，子线程中把id获得并设置一下，然后唤醒Thread类，并执行相应的func函数

## 9
- 感觉Thread类这里不需要阻塞呀？
