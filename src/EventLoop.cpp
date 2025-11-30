#include "EventLoop.h"

#include <atomic>

EventLoop::EventLoop(bool ismainpool)
    : m_pep(std::make_unique<Epoll>()), // 创建epoll
      m_stop(false),                    // 事件循环停止表示为false
      m_ismmainloop(ismainpool),
      m_eventfd(), // 创建 eventfd
      m_pwakechannel(std::make_unique<Channel>(this, std::make_shared<Socket>(m_eventfd.fd()))),
      m_timer(), // 定时器对象，使用Timer类默认的闹钟时间
      m_ptimerchannel(std::make_unique<Channel>(this, std::make_shared<Socket>(m_timer.fd())))
{
    // 设置 事件循环检测到 eventfd 可读之后的回调函数
    m_pwakechannel->setreadeventcb([this]()
                                   { handleWakeUp(); });

    // 将 eventfd 加入到epoll的事件循环检测中
    m_pwakechannel->enablereading();

    // 设置 事件循环检测到定时器超时 之后的回调函数
    m_ptimerchannel->setreadeventcb([this]()
                                    { handleTimer(); });

    // 让事件循环检测定时器的读事件
    m_ptimerchannel->enablereading();
}

EventLoop::~EventLoop()
{
}

// 开启事件循环
void EventLoop::loop()
{
    m_threadid = syscall(SYS_gettid); // 获取事件循环所在线程的线层ID

    while (!m_stop.load())
    {
        int timeout = 10; // 超时时间10ms
        vector<Channel *> channels = m_pep->epollwait(timeout);
        int fdCnt = channels.size();

        if (fdCnt == 0) // 出错或者超时
        {
            if (errno == EINTR) // 信号中断
            {
                continue;
            }
            else if (timeout == 0) // 超时
            {
                m_handletimeout(this);
                continue;
            }
            else if (timeout == -1) // 出错
            {
                return;
            }
        }
        else if (fdCnt > 0)
        {
            for (int i = 0; i < fdCnt; ++i)
            {
                channels[i]->handleevents();
            }

            // 在每一轮事件循环结束后，再处理需要断开的连接
            deleteConnection();
        }
    }
}

// 停止事件循环
void EventLoop::stop()
{
    m_stop.store(true);
    m_eventfd.wakeup(); // 唤醒事件循环
}

// 设置 m_handletimeout
void EventLoop::sethandletimeout(std::function<void(EventLoop *)> func)
{
    m_handletimeout = func;
}

// 将Channel添加到事件循环，或者修改Channel在事件循环上面的事件
void EventLoop::updateChannel(Channel *pchannel)
{
    m_pep->updatechannel(pchannel);
}

// 将Channel从事件循环中删除
void EventLoop::removeChannel(Channel *pchannel)
{
    m_pep->removechannel(pchannel);
}

// 判断当前线程是不是I/O线程，m_threadid里记录的是每个I/O线程的线程号
bool EventLoop::isEventLoopThread()
{
    return m_threadid == syscall(SYS_gettid);
}

// 将任务加入到任务队列中
void EventLoop::addTask(std::function<void()> func)
{
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_taskqueue.push(func); // 工作线程执行
    }

    m_eventfd.wakeup(); // 唤醒事件循环
}

// 回调函数，处理 eventfd 唤醒epoll_wait
void EventLoop::handleWakeUp()
{
    m_eventfd.wait();

    if (!m_ismmainloop) // 从事件循环在任务队列中取出send任务
    {
        // 从任务队列里面取出任务执行，这是在I/O线程中进行的
        std::lock_guard<std::mutex> lock(m_mtx);
        while (!m_taskqueue.empty())
        {
            std::function<void()> func = std::move(m_taskqueue.front()); // I/O线程执行
            func();
            m_taskqueue.pop();
        }
    }
}

// 回调函数，处理定时器事件
void EventLoop::handleTimer()
{
    m_timer.wait();

    if (!m_ismmainloop) // 从事件循环才处理定时器事件
    {
        removeTimeOutConnection(m_timeout);
    }
}

// 有新的连接时，由 TcpServer 调用，往m_lruconnection和m_connectionmap里添加成员
void EventLoop::newConnection(std::shared_ptr<Connection> pConn)
{
    std::lock_guard<std::mutex> lock(m_mtx);

    // 将新的连接添加到m_lruconnection的头部，shared_ptr可以自动转换成weak_ptr
    m_lruconnection.push_front(pConn);

    // 在m_connectionmap里记录Connection对象在m_lruconnection里的迭代器位置
    m_connectionmap[pConn->fd()] = m_lruconnection.begin();
}

// 当Connection连接有I/O事件发生时，由 Connection 调用此函数来“续命”，将Connection连接splice到链表头部
void EventLoop::updateConnection(int fd)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_connectionmap.find(fd);
    if (it != m_connectionmap.end())
    {
        m_lruconnection.splice(m_lruconnection.begin(), m_lruconnection, it->second);
    }
}

// 删除超时的连接
void EventLoop::removeTimeOutConnection(time_t interval)
{
    // 1. 创建一个临时列表，用于存放需要通知 TcpServer 删除的 fd
    std::vector<int> timeoutfds;

    {
        std::lock_guard<std::mutex> lock(m_mtx);
        // 2. 在一个严格的锁作用域内，安全地遍历和和删除 m_lruconnection里面的超时Connection连接
        while (!m_lruconnection.empty())
        {
            auto connWptr = m_lruconnection.back();
            if (auto connSptr = connWptr.lock()) // 尝试提升为shared_ptr,如果提升成功，说明该Connection连接没断开
            {
                if (connSptr->isTimeOut(interval))
                {
                    timeoutfds.push_back(connSptr->fd()); // 收集 fd,稍后从TcpServer持有的Connectionmap里面删除
                    m_connectionmap.erase(connSptr->fd());
                    m_lruconnection.pop_back();
                }
                else // 队尾最久没有发生I/O事件的Connection连接都还么超时，那么所有的Connection都没有超时
                {
                    break;
                }
            }
            else // weak_ptr 提升失败，说明此Connection连接已经被释放
            {
                m_lruconnection.pop_back();
            }
        }
    }

    for (auto fd : timeoutfds)
    {
        m_timerCallback(fd);
    }
}

// 设置m_timerCallback，用来删除TcpServer对象的 map里面的Connection连接
void EventLoop::settimerCallback(std::function<void(int)> func)
{
    m_timerCallback = func;
}

// 设置 m_delayDeleteCallback
void EventLoop::setdelayDeleteCallback(std::function<void(int)> func)
{
    m_delayDeleteCallback = func;
}

// 将Connection的fd添加到m_delayDeleteConnectionfd里
void EventLoop::delayDelete(int fd)
{
    m_delayDeleteConnectionfd.insert(fd);
}

// 删除m_delayDeleteConnectionfd里面所有的连接，并清空m_delayDeleteConnectionfd
void EventLoop::deleteConnection()
{
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        for (auto fd : m_delayDeleteConnectionfd)
        {
            auto it = m_connectionmap.find(fd);
            if (it != m_connectionmap.end())
            {
                m_lruconnection.erase(it->second);
                m_connectionmap.erase(it); // 删除 m_connectionmap 里面已经断开的连接的映射
            }
        }
    }

    // 删除 TcpServer 对象的Connection连接
    for (auto fd : m_delayDeleteConnectionfd)
    {
        m_delayDeleteCallback(fd);
    }

    m_delayDeleteConnectionfd.clear();
}
