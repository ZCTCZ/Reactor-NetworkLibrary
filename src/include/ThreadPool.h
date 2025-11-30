#pragma once
#include "Log.h"

#include <sys/syscall.h> // SYS_gettid
#include <unistd.h>      // syscall 原型
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

class ThreadPool
{
public:
    // 启动 num 个线程
    ThreadPool(size_t num, const std::string& type = "IO");

    // 返回线程池的大小
    size_t size();

    // 将任务添加到任务队列中
    template <typename F>
    void AddTask(F &&f)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasksqueue.emplace(std::forward<F>(f));
        m_condition.notify_one();
    }
    
    // 停止线程池里的所有线程
    void stop();

    // 在析构函数中停止线程池的工作
    ~ThreadPool();

private:
    std::vector<std::thread> m_threads;             // 线程池
    std::queue<std::function<void()>> m_tasksqueue; // 任务队列
    std::mutex m_mutex;                              
    std::condition_variable m_condition;            
    std::atomic<bool> m_stop;                       // 控制线程池是否停止工作的按钮
    std::string m_type;                             // 线程池的类型，I/O线程或者WORK线程
};