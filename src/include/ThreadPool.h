#pragma once

#include <cstddef>
#include <future>
#include <memory>
#include <sys/syscall.h> // SYS_gettid
#include <tuple>
#include <unistd.h> // syscall 原型
#include <utility>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    /// 任务类型
    using TaskType = std::packaged_task<void()>;

    // 启动 num 个线程
    ThreadPool(size_t num = std::thread::hardware_concurrency(), const std::string& type = "IO");

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 返回线程池的大小
    std::size_t size();

    /// 返回空闲线程的个数
    std::size_t FreeThreadsNums();

    // 将任务添加到任务队列中
    template <typename F, typename... Args>
    auto AddTask(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {
        using RetType = decltype(f(args...));

        if (m_stop.load()) {
            return std::future<RetType>();
        }

        auto bound_args = std::tuple<Args...>(std::forward<Args>(args)...);
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            [f = std::forward<F>(f), bound_args = std::move(bound_args)]() mutable {
                return std::apply(f, bound_args);
            });

        auto ret = task->get_future();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasksqueue.emplace([task]() { (*task)(); });
        }

        m_condition.notify_one();
        return ret;
    }

    // 停止线程池里的所有线程
    void stop();

    // 在析构函数中停止线程池的工作
    ~ThreadPool();

private:
    // 启动线程池
    void Run(std::size_t nums);

    std::vector<std::thread> m_threads; // 线程池
    std::queue<TaskType> m_tasksqueue; // 任务队列
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop{false}; // 控制线程池是否停止工作的按钮
    std::atomic<std::size_t> m_free_nums; // 空闲线程个数
    std::string m_type; // 线程池的类型，I/O线程或者WORK线程
};
