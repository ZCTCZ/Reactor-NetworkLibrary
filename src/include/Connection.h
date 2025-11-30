#pragma once

#include "EventLoop.h"
#include "Buffer.h"
#include "TimesTamp.h"

#include <memory>
#include <atomic>
#include <functional>

class Channel; //向前声明Channel类
class EventLoop; //向前声明EventLoop类
class Socket;


// 管理 通信套接字、和通信套接字关联的Channel
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(std::shared_ptr<Socket> psocket, EventLoop* ploop);
    ~Connection();

    // 关闭TCP连接时，析构该条TCP连接通信套接字所属的Connection对象 的回调函数
    void closeconnection();

    // 设置 m_closeconnectioncb
    // void setcloseconnection(std::function<void(std::shared_ptr<Connection>)> func);

    // 设置 m_handlemessagecb
    void sethandlemessage(std::function<void(std::shared_ptr<Connection> pConn, Buffer*)> func);

    // 设置 m_sendcompletecb
    void setsendcomplete(std::function<void(std::shared_ptr<Connection>)> func);

    // 处理 已存在的TCP连接的客户端I/O 的回调函数
    void onmessage();

    // 将数据放到Connection对象的写缓冲区里，并注册写事件
    void send(const std::string& msg);

    // 将 Connection 写缓冲区里的数据发送到内核的写缓冲区
    void sendto();

    // 获取 通信套接字fd
    int fd() const;

    // 将Connection对象的Channel 添加到事件循环中，让epoll监听它的读事件
    void addToEpoll();

    // 判断当前连接是否超时
    bool isTimeOut(time_t interval);


private:
    std::shared_ptr<Socket> m_psocket;
    EventLoop* m_ploop;
    std::shared_ptr<Channel> m_pchannel;
    Buffer m_inputbuf; // 接收缓冲区
    Buffer m_outputbuf; // 发送缓冲区
    std::atomic<bool> m_disconnect; // 记录当前Connection连接是否断开
    TimesTamp m_lasttime; // 时间戳对象
    bool m_istimeout = false; // 记录当前Connection连接是否超时

    std::function<void(std::shared_ptr<Connection>, Buffer*)> m_handlemessagecb; // 处理客户端发送过来的数据的回调函数
    std::function<void(std::shared_ptr<Connection>)> m_sendcompletecb; // 当数据发送给客户端后的回调函数

    // 将待发送的数据msg写入Connection对象的写缓冲区
    void writeTo(const std::string& msg);
};