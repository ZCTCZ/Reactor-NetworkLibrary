// 回声服务器类
#pragma once

#include "TcpServer.h"

class EchoServer
{
public:
    EchoServer(const std::string &ip, uint16_t port, uint16_t subthreads = 3, uint16_t workthreads = 3);
    ~EchoServer();

    // 启动服务器
    void Start();

    // 关闭服务器
    void Stop();

    // 处理客户端发送过来的消息
    void HandleOnMessage(std::shared_ptr<Connection> pConn, Buffer* buffer);

    // 处理新的客户端连接请求
    void HandleNewConnection(std::shared_ptr<Socket> pClientSocket);

    // 处理客户端断开连接的请求
    void HandleDeleteConnection(int fd);

    // 处理服务器将消息发送给客户端之后的业务逻辑
    void HandleSendComplete(std::shared_ptr<Connection> pConn);

    // 处理事件循环检测中发生超时的情况
    void HandleEventLoopTimeout(EventLoop* peloop);

    // 处理具体业务
    void OnMessage(std::shared_ptr<Connection> pConn, std::string msg);

private:
    TcpServer m_tcpserver; // 服务器类
    ThreadPool m_workthreads; // 工作线程池
};