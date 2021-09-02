# C++并发编程简介

C++11 引入了对多线程和lambda表达式的支持

# thread
创建线程
```cpp
#include<thread>
int main(int argc, char** argv) {
  std::thread t([](){
    std::cout << "hello world." << std::endl
  });
  t.join();
  return 0;
}
```

# mutex
多个线程通过互斥量访问关键区  
- std::mutex时基本的互斥量类，可以通过其lock()和unlock()进行上锁解锁。
- 但我们一般不手动操作，使用std::lock_guard管理，析构时自动释放，符合RAII
- std::unique_lock比std::lock_guard更加灵活，可以手动unlock和lock
```cpp
#include<thread>
#include<mutex>
int v = 1;
void critical_section(int set_v) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    v = set_v;
    //mutext 自动释放
}
int main(int argc, char** argv) {
    std::thread t1(critical_section, 2), t2(critical_section, 3);
    t1.join();
    t2.join();
    std::cout << v << std::endl;
    return 0;
}
```

# future
使用future获取线程执行结果
- 使用std::packaged_task封装一个任务，然后交给thread执行
- 使用std::async直接执行一个异步任务  

```cpp
#include <iostream>
#include <future>
#include <thread>

int main() { 
    std::packaged_task<int()> task([](){return 7;});
    
    std::future<int> result = task.get_future(); 
    std::thread(std::move(task)).detach();// 在一个线程中执行 task
    std::cout << "waiting..."; 
    result.wait();   
    std::cout << "done!" << std:: endl << "future result is " << result.get() << std::endl;

    //==================================
    auto is_prime = [](int x) {
      for (int i = 2; i < (x); ++i)
        if (x % i == 0) return false;
      return true;
    };
    constexpr int num = 444444443;
    std::future<bool> fut = std::async(is_prime, num);

    cout << "checking, please wait";
    std::chrono::milliseconds span(100);
    while (fut.wait_for(span) == std::future_status::timeout) 
      cout << '.' << std::flush;

    bool x = fut.get();  // retrieve return value
    cout << "\n" << num << " " << (x ? "is" : "is not") << " prime.\n";
    return 0;
}
```

# condition_variable
使用条件变量condition_variable进行线程间通信 
- notify_all(), notify_one()进行通知
- wait(mutx),为了避免虚假唤醒，一般需要检查某个标志

```cpp
#include <queue>
#include <chrono>
#include <mutex>
#include <thread>
#include <iostream>
#include <condition_variable> 

int main() {
    std::queue<int> produced_nums;
    std::mutex mtx;
    std::condition_variable cv;
    bool notified = false;  // 通知信号 
    // 生产者
    auto producer = [&]() {
        for (int i = 0; ; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(900));
            std::unique_lock<std::mutex> lock(mtx);
            std::cout << "producing " << i << std::endl;
            produced_nums.push(i);
            notified = true;
            cv.notify_all(); // 此处也可以使用 notify_one
        }
    };
    // 消费者
    auto consumer = [&]() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            while (!notified) {  // 避免虚假唤醒
                cv.wait(lock);
            }
            // 短暂取消锁，使得生产者有机会在消费者消费空前继续生产
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 消费者慢于生产者
            lock.lock();
            while (!produced_nums.empty()) {
                std::cout << "consuming " << produced_nums.front() << std::endl;
                produced_nums.pop();
            }
            notified = false;
        }
    };

    // 分别在不同的线程中运行
    std::thread p(producer);
    std::thread cs[2];
    for (int i = 0; i < 2; ++i) {
        cs[i] = std::thread(consumer);
    }
    p.join();
    for (int i = 0; i < 2; ++i) cs[i].join(); 
    return 0;
}
```

