#pragma once

#include "Socket.h"
#include "EventLoop.h"
#include "ThreadPool.h"
#include "Acceptor.h"
#include "Buffer.h"

#include <unordered_map>

class TcpServer
{
public:
    TcpServer(const std::string& ip, uint16_t port, uint16_t nums = 3);
    ~TcpServer();

    // 启动服务器
    void start();

    // 关闭服务器
    void stop();

    // 创建Connection对象
    void createconnection(const std::shared_ptr<Socket>& pClientSocket);

    // 从clientConnectionMap中删除指定主键的Connection对象
    void deleteconnection(int fd);

    // 处理Connection对象接收到的客户端发送过来的一条完整的数据
    void handlemessage(std::shared_ptr<Connection> pConn, Buffer* buffer); 

    // 当Connection输出缓冲区里的数据已经全部发送出去后，调用该函数，处理之后的逻辑
    void sendcomplete(std::shared_ptr<Connection>);

    // 处理事件循环检测中发生超时的情况
    void eventlooptimeout(EventLoop* peloop);

    // 给 函数对象 m_handlecreateconnectioncb 赋值
    void sethandlecreateconnectioncb(std::function<void(const std::shared_ptr<Socket>)> func);

    // 给 函数对象 m_handledeleteconnectioncb 赋值
    void sethandledeleteconnectioncb(std::function<void(int)> func);

    // 给 函数对象 m_handlemessage 赋值
    void sethandlemessage(std::function<void(std::shared_ptr<Connection>, Buffer*)> func);

    // 给 函数对象 m_handlesendcomplete 赋值
    void sethandlesendcomplete(std::function<void(std::shared_ptr<Connection>)> func);

    // 给 函数对象 m_handleeventlooptimeout 赋值
    void sethandleeventlooptimeout(std::function<void(EventLoop*)> func);

    // 从 m_clientConnectionMap 移除超时的Connection连接，由EventLoop对象通过回调的方式调用
    void removeTimeOutConnection(int fd);

private:
    std::unique_ptr<EventLoop> m_pmainloop;               // 主事件循环, 只负责客户端建立新连接的请求
    Acceptor m_acceptor;                                  // 连接器
    std::vector<std::unique_ptr<EventLoop>> m_psubloop;   // 从事件循环，负责已建立连接的客户端的I/O请求
    uint16_t m_threadsnums;                               // 子线程个数，同时也是从事件循环的个数
    ThreadPool m_threadpool;                              // 线程池，里面的每个线程负责运行一个事件循环
    std::unordered_map<int, std::shared_ptr<Connection>> m_clientConnectionMap; // 记录套接字和Connection连接的映射
    std::mutex m_mtx;

    // 下面的 5 个回调函数，都是用于TCPServer类调用它的上层类的函数
    std::function<void(const std::shared_ptr<Socket>)> m_handlecreateconnectioncb; // 回调函数，建立新的Connection连接
    std::function<void(int)> m_handledeleteconnectioncb; // 回调函数，删除Connection连接
    std::function<void(std::shared_ptr<Connection>, Buffer*)> m_handlemessage; // 回调函数，处理客户端发送过来的数据
    std::function<void(std::shared_ptr<Connection>)> m_handlesendcomplete; // 回调函数，完成处理结果发送给客户端之后的业务逻辑
    std::function<void(EventLoop*)> m_handleeventlooptimeout; // 回调函数，处理事件循环超时
};