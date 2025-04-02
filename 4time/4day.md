## 1
- bool recurring-这个参数表示定时器是否是周期性的：
- true：表示定时器会重复触发（即每隔 ms 毫秒触发一次）。
- false：表示定时器仅触发一次。
## 2
- std::set< std::shared_ptr< Timer >, Timer::Comparator> m_timers---------最小堆
- m_timers 是一个 std::set 容器，std::set< T, Comparator > 是一个有序集合，它的元素类型为 T，并且使用 Comparator 进行排序，这里 T = std::shared_ptr<Timer>，意味着集合中的元素是**Timer 的智能指针**（std::shared_ptr< Timer >），Comparator = Timer::Comparator，这个是 set 用来比较元素顺序的规则，决定哪些 Timer 在前，哪些在后
- Comparator这个结构体里面的函数名字必须是operator()，并且返回值是bool类型

## 3
- bool operator()(const std::shared_ptr<Timer>& a,const std::shared_ptr<Timer>& b)const;这李最后一个const的作用是什么?
- 末尾的 const 是用来保证这个比较函数不会修改 Comparator 结构体的成员变量

## 4
- m_next = now + std::chrono::milliseconds(m_ms);为什么需要变化std::chrono::milliseconds(m_ms)
- m_ms 是 uint64_t（整数），std::chrono::milliseconds(m_ms) 会把它转换成 std::chrono::milliseconds 类型，使得 now + ... 计算时，单位是毫秒

## 5
- std::unique_lock<std::shared_mutex> write_lock(m_manager->m_mutex);
- 多个线程可以同时读取，但只有一个线程能修改，避免数据竞争

## 6
- ~0ull：表示无穷大 (0xFFFFFFFFFFFFFFFF)，即 18446744073709551615（uint64_t 最大值）。

## 7
- static_cast<目标类型>(表达式)
- duration.count() 返回的是 long long 类型（取决于平台）。
- static_cast<uint64_t> 将其转换为 uint64_t 类型，保证返回值符合 getNextTimer() 的返回类型。

