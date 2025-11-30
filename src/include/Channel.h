#pragma once

#include "Log.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Connection.h"

#include <sys/epoll.h>
#include <unordered_map>
#include <memory>
#include <functional>

class EventLoop;// 向前声明EventLoop类

class Channel
{
public:
    Channel(EventLoop* pElp, std::shared_ptr<Socket> psocket);
    ~Channel() = default;

    // 返回 m_psocket->fd()
    int getfd() const;

    // 设置 m_isinepoll 成员
    void setisinepoll();

    // 返回 m_isinepoll 成员
    bool getisinepoll() const;

    // 设置 m_events 成员
    void setevents(uint32_t ev);

    // 返回 m_events 成员
    uint32_t getevents() const;

    // 设置 happenevents 成员
    void sethappenevents(uint32_t ev);

    // 返回 happenevents 成员
    uint32_t gethappenevents() const;

    // 设置 ET模式
    void setepollet();

    // 让 epoll 监视 Channel 对象的读事件， 即注册读事件
    void enablereading();

    // 让 epoll 停止监视 Channel 对象的读事件
    void disablereading();

    // 让 epoll 监视 Channel 对象的写事件， 即注册写事件
    void enablewriting();

    // 让 epoll 停止监视 Channel 对象的写事件
    void disablewriting();

    //  取消监视的全部事件
    void disableall();

    // 从事件循环中删除Channel
    void remove();

    // 处理 epoll监视到已经发生的事件
    void handleevents();

    // 设置 m_readeventcb
    void setreadeventcb(const std::function<void()>& func);

    // 设置 m_closecb
    void setcloseconnectioncb(const std::function<void()>& func);

    // 设置 m_writeeventcb
    void setwriteeventcb(const std::function<void()>& func);

private:
    std::shared_ptr<Socket> m_psocket;          // 每一个Channel对象唯一对应一个Socket对象
    EventLoop* m_pelp;                          // 每一个Channel对象唯一对应一个EventLoop对象，但是每一个EventLoop对象对应多个Channel对象
    bool m_isinepoll = false;                   // 记录当前Channel对象是否加入到了Epoll对象的监测中
    uint32_t m_events = 0;                      // 需要监视的事件
    uint32_t m_happenevents = 0;                // epoll监视到已经发生的事件
    std::function<void()> m_readeventcb;        // epoll监视到的EPOLLIN类型的事件的回调函数
    std::function<void()> m_closeconnectioncb;  // 析构Connection对象的回调函数
    std::function<void()> m_writeeventcb;       // epoll监视到EPOLLOUT类型的事件的回调函数
};