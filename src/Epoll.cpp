#include "Epoll.h"

#include <unistd.h>
#include <cstring>
#include <errno.h>

Epoll::Epoll()
    : m_epfd(epoll_create(1)) 
    {
        if (m_epfd == -1)
        {
            LOG(error) << "epoll_create() err";
        }
    }

Epoll::~Epoll()
{
    ::close(m_epfd);
}

int Epoll::updatechannel(Channel *pchannel)
{
    struct epoll_event ev;
    ev.data.ptr = pchannel;
    ev.events = pchannel->getevents();

    int ret = 0;

    if (pchannel->getisinepoll()) // 当前Channel对象已经被epoll监视
    {
        ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, pchannel->getfd(), &ev);
        if (ret == -1)  
        {
            LOG(error) << "epoll_ctl(EPOLL_CTL_MOD) err";
        }
    }
    else
    {
        pchannel->setisinepoll();
        ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, pchannel->getfd(), &ev);
        if (ret == -1)
        {
            LOG(error) << "epoll_ctl(EPOLL_CTL_ADD) err";
        }
    }

    return ret;
}

// 将通信套接字从epoll中移除
int Epoll::removechannel(Channel* pchannel)
{
    int ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, pchannel->getfd(), nullptr);
    if (ret == -1)
    {
        LOG(error) << "epoll_ctl(EPOLL_CTL_DEL) err";
    }
    return ret;
}

// 返回m_epfd
int Epoll::get() const
{
    return m_epfd;
}

/**
 * @brief 等待epoll所监听的事件的发生,设置超时时间
 *
 * @param timeout 超时时间，传入传出参数。若传出值为-1，说明发生了错误，若传出值为0，说明没有发生错误。
 * @return vector<struct epoll_event> 若为空，说明发生了错误或者超时时间到，需要结合timeout的传出值来区分。
 */
vector<Channel*> Epoll::epollwait(int& timeout)
{
    vector<Channel*> retvec;
    memset((void *)&m_evs, 0, MAXEVENTS);
    int cnt = epoll_wait(m_epfd, m_evs, MAXEVENTS, timeout);
    if (cnt == -1) // 出错
    {
        if (errno != EINTR)
        {
            timeout = -1;
            LOG(error) << "epoll_wait() err";
        }
        return retvec;
    }
    else if (cnt == 0) // 超时
    {
        timeout = 0;
        return retvec;
    }
    else
    {
        timeout = 0;
        retvec.reserve(cnt);
        for (int i = 0; i < cnt; ++i)
        {
            Channel* channel = static_cast<Channel*>(m_evs[i].data.ptr);
            channel->sethappenevents(m_evs[i].events);
            retvec.push_back(channel);
        }
        return retvec;
    }
}