#pragma once

#include <sys/eventfd.h>
#include <unistd.h>

// 封装eventfd的类
class EventFd
{
public:
    EventFd();
    ~EventFd();
    
    // 获取eventfd
    int fd() const;

    // 唤醒事件循环
    void wakeup();

    // 从eventfd里面读取数据，防止事件循环被重复多次唤醒
    void wait();

private:
    int m_eventfd;
};