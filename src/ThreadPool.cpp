#include "ThreadPool.h"

// 启动 num 个线程
ThreadPool::ThreadPool(size_t num, const std::string &type)
    : m_stop(false), 
      m_type(type)
{
    for (size_t i = 0; i < num; ++i)
    {   
        m_threads.emplace_back(std::thread([this]()
        {
           {
            LOG(info) <<  "create " << m_type << " thread(" << syscall(SYS_gettid) << ")";
            // std::cout << "create " << m_type << " thread(" << syscall(SYS_gettid) << ")" << std::endl;
           }
            
            while (true)
            {
                std::function<void()> task; // 用于存放出队任务
                
                { //////////////锁的作用域开始////////////////////////////////////
                    std::unique_lock<std::mutex> lock(m_mutex);

                    // 等待从任务队列中取出任务, 每 50ms 醒来一次检查，防止忙等
                    while (!m_condition.wait_for(lock, std::chrono::milliseconds(50), [this](){
                            return (m_stop.load() || !m_tasksqueue.empty());}))
                    {
                        if (m_stop.load() && m_tasksqueue.empty())
                        {
                            return ;
                        }
                    }

                    if (m_stop.load() && m_tasksqueue.empty()) // 当关闭线程池并且任务队列为空时，退出线程
                    {
                        return ;
                    }

                    // 出队任务队列的队首任务，让当前线程执行这个任务
                    task = std::move(m_tasksqueue.front());
                    m_tasksqueue.pop();
                    
                    // // 析构函数调用后,如果任务队列里有任务。每次消耗掉一个任务之后，唤醒一个线程。
                    // if (m_stop .load() && !m_tasksqueue.empty())
                    // {
                    //     m_condition.notify_one();
                    // }
                } //////////////锁的作用域结束////////////////////////////////////
            
                // std::cout << m_type << " thread is: " << syscall(SYS_gettid) << std::endl;
                task();
            } 
        }));
    }
}

// 返回线程池的大小
size_t ThreadPool::size()
{
    return m_threads.size();
}

void ThreadPool::stop()
{
    m_stop.store(true);       // 将线程池关闭按钮设置为true
    m_condition.notify_all(); // 唤醒所有的线程

    // 等待所有线程结束后再关闭线程池
    for (auto &e : m_threads)
    {
        e.join();
    }
}

// 在析构函数中停止线程池的工作
ThreadPool::~ThreadPool()
{
    if (!m_stop)
    {
        stop();
    }
}