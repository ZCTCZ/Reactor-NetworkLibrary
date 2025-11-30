#include "Channel.h"

#include <sstream>
#include <iostream>
#include <cstring>

Channel::Channel(EventLoop* pElp, std::shared_ptr<Socket> psocket)
    : m_pelp(pElp), m_psocket(psocket) {}

// 获取当前时间，格式 Y-M-D h:m:s
// std::string current_time_str()
// {
//     // 1. 获取当前系统时钟（精度到秒）
//     auto now = std::chrono::system_clock::now();
//     std::time_t t = std::chrono::system_clock::to_time_t(now);

//     // 2. 分解为本地日历时间
//     std::tm tm{};
// #if defined(_WIN32)
//     localtime_s(&tm, &t);
// #else
//     localtime_r(&t, &tm); // POSIX
// #endif

//     // 3. 格式化输出：年-月-日 时:分:秒
//     std::ostringstream oss;
//     oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
//     return oss.str();
// }

// 返回 m_psocket->fd()
int Channel::getfd() const
{
    return m_psocket->fd();
}

// 设置 m_isinepoll 成员
void Channel::setisinepoll()
{
    m_isinepoll = true;
}

// 返回 m_isinepoll 成员
bool Channel::getisinepoll() const
{
    return m_isinepoll;
}

// 设置 m_events 成员
void Channel::setevents(uint32_t ev)
{
    m_events |= ev;
}

// 返回 m_events 成员
uint32_t Channel::getevents() const
{
    return m_events;
}

// 设置 happenevents 成员
void Channel::sethappenevents(uint32_t ev)
{
    m_happenevents = ev; // 应该是直接赋值，而不是位或
}

// 返回 happenevents 成员
uint32_t Channel::gethappenevents() const
{
    return m_happenevents;
}

// 设置 ET模式
void Channel::setepollet()
{
    m_events |= EPOLLET;
}

// 让 epoll 监视 Channel 对象的读事件
void Channel::enablereading()
{
    m_events |= EPOLLIN;
    m_pelp->updateChannel(this);
}

// 让 epoll 停止监视 Channel 对象的读事件
void Channel::disablereading()
{
    m_events &= ~EPOLLIN;
    m_pelp->updateChannel(this);
}

// 让 epoll 监视 Channel 对象的写事件， 即注册写事件
void Channel::enablewriting()
{
    m_events |= EPOLLOUT;
    m_pelp->updateChannel(this);
}

// 让 epoll 停止监视 Channel 对象的写事件
void Channel::disablewriting()
{
    m_events &= ~EPOLLOUT;
    m_pelp->updateChannel(this);
}

//  取消监视的全部事件
void Channel::disableall()
{
    m_events = 0;
    m_pelp->updateChannel(this);
}

// 从事件循环中删除Channel
void Channel::remove()
{
    disableall();
    m_pelp->removeChannel(this);
}

// 处理 epoll监视到已经发生的事件
void Channel::handleevents()
{
    if (m_happenevents & EPOLLRDHUP) // 对端客户端关闭了连接
    {
        std::ostringstream oss;
        oss << "ip=" << m_psocket->getip()
            << ",port=" << m_psocket->getport()
            << ",fd=" << m_psocket->fd()
            << " disconnected";
        LOG(info) << oss.str();

        m_closeconnectioncb();
        return ;
    }

    if (m_happenevents & (EPOLLIN | EPOLLPRI)) // 读缓冲区里面有数据
    {
        m_readeventcb(); 
    }

    if (m_happenevents & EPOLLOUT) // 可以向对端发送数据
    {
        m_writeeventcb();
    }
}

// 设置 m_readeventcb
void Channel::setreadeventcb(const std::function<void()> &func)
{
    m_readeventcb = func;
}

// 设置 m_closecb
void Channel::setcloseconnectioncb(const std::function<void()> &func)
{
    m_closeconnectioncb = func;
}

// 设置 m_writeeventcb
void Channel::setwriteeventcb(const std::function<void()>& func)
{
    m_writeeventcb = func;
}