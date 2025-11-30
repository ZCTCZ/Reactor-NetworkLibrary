#pragma once

#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

// 管理服务器监听套接字和监听套接字对应的Channel(连接器)
class Acceptor
{
public:
    Acceptor(EventLoop* pLoop, const std::string &ip, uint16_t port);
    ~Acceptor();

    // 处理客户端建立新连接请求的回调函数
    void onconnect();

    // 设置m_onconnectcb
    void setonconnectcb(std::function<void(std::shared_ptr<Socket>)> fn);
private:
    EventLoop* m_ploop; // 主事件循环
    std::shared_ptr<Socket> m_psocket;
    std::shared_ptr<Channel> m_pchannel;
    std::function<void(std::shared_ptr<Socket>)> m_onconnectcb; // 回调函数，用来创建Connection对象，调用TcpServer类的createconnection函数
};