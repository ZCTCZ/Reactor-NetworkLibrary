#pragma once

#include "Channel.h"
#include "Log.h"

#include <sys/epoll.h>
#include <vector>

using std::vector;
class Channel; // 向前声明Channel类

class Epoll
{
public:
    Epoll();

    ~Epoll();

    // 更新Channel对象的m_events属性
    int updatechannel(Channel* pchannel);

    // 将通信套接字从epoll中移除
    int removechannel(Channel* pchannel);

    // 返回m_epfd
    int get() const;

    /**
     * @brief 等待epoll所监听的事件的发生,设置超时时间
     * 
     * @param timeout 超时时间，传入传出参数。若传出值为-1，说明发生了错误，若传出值为0，说明没有发生错误。
     * @return vector<struct epoll_event> 若为空，说明发生了错误或者超时时间到，需要结合timeout的传出值来区分。
     */
    vector<Channel*> epollwait(int& timeout);

    static const int MAXEVENTS = 1024; // 单个epoll一次最多能处理的事件数量
private:
    int m_epfd = -1;
    struct epoll_event m_evs[MAXEVENTS]; // 存放epoll_wait返回的事件
};