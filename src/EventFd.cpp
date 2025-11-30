#include "EventFd.h"

EventFd::EventFd()
    :m_eventfd(eventfd(0, EFD_NONBLOCK))
{}

EventFd::~EventFd()
{
    ::close(m_eventfd);
}

int EventFd::fd() const
{
    return m_eventfd;
}

// 唤醒事件循环
void EventFd::wakeup()
{
    uint64_t val;
    write(m_eventfd, &val, 8); // 向m_wakeupfd 里面写入数据，会唤醒epoll_wait
}

// 从eventfd里面读取数据，防止事件循环被重复多次唤醒
void EventFd::wait()
{
    uint64_t buf;
    read(m_eventfd, &buf, sizeof(buf));
}