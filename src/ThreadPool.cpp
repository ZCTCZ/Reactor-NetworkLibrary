#include "ThreadPool.h"
#include <chrono>
#include <cstddef>
#include <mutex>
#include <utility>
#include "Log.h"

// 启动 num 个线程
ThreadPool::ThreadPool(size_t nums, const std::string& type) : m_stop(false), m_free_nums(nums), m_type(type)
{
    m_threads.reserve(nums);
    Run(nums);
}

void ThreadPool::Run(std::size_t nums)
{
    for (int i = 0; i < nums; ++i) {
        m_threads.emplace_back([this]() {
            {
                LOG(info) << "create " << m_type << " thread(" << syscall(SYS_gettid) << ")";
                // std::cout << "create " << m_type << " thread(" << syscall(SYS_gettid) << ")" << std::endl;
            }

            while (!m_stop.load()) {
                {
                    TaskType task;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_condition.wait_for(lock, std::chrono::milliseconds(50),
                                             [this]() { return m_stop.load() || !m_tasksqueue.empty(); });

                        // 当 m_stop 为真 并且 任务队列为空，才结束线程
                        if (m_stop.load() && m_tasksqueue.empty()) {
                            return;
                        }

                        task = std::move(m_tasksqueue.front());
                        m_tasksqueue.pop();
                    }

                    m_free_nums.fetch_sub(1);
                    task(); // 执行任务
                    m_free_nums.fetch_add(1);
                }
            }
        });
    }
}

// 返回线程池的大小
size_t ThreadPool::size() { return m_threads.size(); }

// 返回线程池里空闲线程数量
std::size_t ThreadPool::FreeThreadsNums() { return m_free_nums.load(); }

void ThreadPool::stop()
{
    m_stop.store(true); // 将线程池关闭按钮设置为true
    m_condition.notify_all(); // 唤醒所有的线程

    // 等待所有线程结束后再关闭线程池
    for (auto& e : m_threads) {
        if (e.joinable()) {
            e.join();
        }
    }
}

// 在析构函数中停止线程池的工作
ThreadPool::~ThreadPool()
{
    if (!m_stop) {
        stop();
    }
}
