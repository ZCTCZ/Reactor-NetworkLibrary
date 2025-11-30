#pragma once

#include "Epoll.h"
#include "EventFd.h"
#include "Timer.h"
#include "Connection.h"

#include <functional>
#include <memory>
#include <sys/syscall.h> // SYS_gettid
#include <unistd.h>      // syscall 原型
#include <queue>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <list>
#include <atomic>
#include <unordered_set>

class Channel;
class Epoll;
class Connection;

class EventLoop
{
public:
    EventLoop(bool ismainloop);
    ~EventLoop();
    
    // 开启事件循环
    void loop();

    // 停止事件循环
    void stop();

    // 将Channel添加到事件循环，或者修改Channel在事件循环上面的事件
    void updateChannel(Channel* pchannel);

    // 将Channel从事件循环中删除
    void removeChannel(Channel* pchannel);

    // 设置 m_handletimeout
    void sethandletimeout(std::function<void(EventLoop*)> func);

    // 判断当前线程是不是I/O线程，用于和工作线程区分
    bool isEventLoopThread();

    // 将任务加入到任务队列中
    void addTask(std::function<void()> func);

    // 处理 eventfd 唤醒epoll_wait
    void handleWakeUp();

    // 处理定时器事件
    void handleTimer(); 

    // 有新的连接时，由 TcpServer 调用，往m_lruconnection和m_connectionmap里添加成员
    void newConnection(std::shared_ptr<Connection> pConn);

    // 当Connection连接有I/O事件发生时，由 Connection 调用此函数来“续命”，将Connection连接splice到链表头部
    void updateConnection(int fd);

    // 删除 m_lruconnection 里面超时的连接
    void removeTimeOutConnection(time_t interval);

    // 设置m_timerCallback
    void settimerCallback(std::function<void(int)> func);

    // 设置 m_delayDeleteCallback
    void setdelayDeleteCallback(std::function<void(int)> func);

    // 将Connection的fd添加到m_delayDeleteConnectionfd里
    void delayDelete(int fd);

    // 删除m_delayDeleteConnectionfd里面所有的连接，并清空m_delayDeleteConnectionfd
    void deleteConnection();

private:
    std::unique_ptr<Epoll> m_pep; // 封装了Epoll
    std::atomic<bool> m_stop; // 事件循环停止的标志
    
    pthread_t m_threadid; // 当前事件循环所在的线程的线程ID
    bool m_ismmainloop; // 当前事件循环是 主事件循环(true)， 还是 从事件循环(false) 的表示
    std::queue<std::function<void()>> m_taskqueue; // 任务队列，放工作线程传给I/O线程的send任务
    std::mutex m_mtx;
    
    EventFd m_eventfd; // 封装了eventfd,用于唤醒事件循环
    std::unique_ptr<Channel> m_pwakechannel; // eventfd所对应的Channel

    Timer m_timer; // 定时器对象
    std::unique_ptr<Channel> m_ptimerchannel; // 定时器所对应的Channel
    const int m_timeout = 300; // 超时时间，默认300s。每隔300s，事件循环就被唤醒一次。

    std::list<std::weak_ptr<Connection>> m_lruconnection; // 按活跃度排序的连接列表，头部是最近活跃的Connection连接
    std::unordered_map<int, std::list<std::weak_ptr<Connection>>::iterator> m_connectionmap;  // 从 fd 快速定位到 list 中的节点
    std::unordered_set<int> m_delayDeleteConnectionfd; // 记录了每轮事件循环后需要延迟删除的Connection连接的fd

    std::function<void(EventLoop*)> m_handletimeout; // 回调函数， 处理事件循环发生超时
    std::function<void(int)> m_timerCallback; // 回调函数，定时器触发后执行，调用cpServer类的removeTimeOutConnection函数，用来删除TcpServer管理的Connection连接
    std::function<void(int)> m_delayDeleteCallback; // 回调函数，每轮事件循环后执行，调用TcpServer类的deleteconnection函数，用来删除TcpServer管理的Connection连接
};