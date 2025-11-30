#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, uint16_t nums)
    : m_pmainloop(std::make_unique<EventLoop>(true)), // 创建主事件循环
      m_acceptor(m_pmainloop.get(), ip, port), // 创建连接器
      m_threadsnums(nums), // 设置从事件循环的个数（I/O线程的个数）
      m_threadpool(m_threadsnums, "IO") // 创建I/O线程池，线程池里的每个线程都运行着一个从事件循环
{
    m_acceptor.setonconnectcb([this](std::shared_ptr<Socket> pClientSocket)
                              { createconnection(pClientSocket); });

    m_pmainloop->sethandletimeout([this](EventLoop *peloop)
                                  { eventlooptimeout(peloop); });

    // 创建从事件循环
    for (int i = 0; i < m_threadsnums; ++i)
    {
        m_psubloop.emplace_back(std::make_unique<EventLoop>(false)); // 创建从事件循环
        m_psubloop[i]->sethandletimeout([this](EventLoop *peloop)
                                        { eventlooptimeout(peloop); });

        m_psubloop[i]->settimerCallback([this](int fd)
                                        { removeTimeOutConnection(fd); });

        m_psubloop[i]->setdelayDeleteCallback([this](int fd)
                                        { deleteconnection(fd); });

        // 将开启事件循环检测的函数放到线程池的任务队列里
        m_threadpool.AddTask([this, i]()
                             { m_psubloop[i]->loop(); });
    }
}

TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
    m_pmainloop->loop();
}

// 停止事件循环
void TcpServer::stop()
{
    // 停止主事件循环
    m_pmainloop->stop();

    // 停止从事件循环
    for (auto &e : m_psubloop)
    {
        e->stop();
    }

    // 停止I/O线程池，里面的线程负责运行从事件循环
    m_threadpool.stop();
}

// 创建Connection对象
void TcpServer::createconnection(const std::shared_ptr<Socket> &pClientSocket)
{
    int fd = pClientSocket->fd();

    {
        // 操作 m_clientConnectionMap 需要加锁
        std::lock_guard<std::mutex> lock(m_mtx);
        m_clientConnectionMap[fd] = std::make_shared<Connection>(pClientSocket, m_psubloop[pClientSocket->fd() % m_threadsnums].get());
    }

    m_clientConnectionMap[fd]->sethandlemessage([this](std::shared_ptr<Connection> pConn, Buffer* buffer)
                                                { handlemessage(pConn, buffer); });
    m_clientConnectionMap[fd]->setsendcomplete([this](std::shared_ptr<Connection> pConn)
                                               { sendcomplete(pConn); });

    // 让从事件循环记录新创建的Connection对象
    m_psubloop[pClientSocket->fd() % m_threadsnums]->newConnection(m_clientConnectionMap[fd]);

    // 在Connection对象创建出来之后，再让事件循环检测它的读事件。按照先创建，再激活的原则，防止竞态条件出现。
    m_clientConnectionMap[fd]->addToEpoll();

    if (m_handlecreateconnectioncb)
        m_handlecreateconnectioncb(pClientSocket);
}

// 从clientConnectionMap中删除指定主键的Connection对象
void TcpServer::deleteconnection(int fd)
{
    {
        // 操作 m_clientConnectionMap 需要加锁
        std::lock_guard<std::mutex> lock(m_mtx);
        m_clientConnectionMap.erase(fd);
    }

    if (m_handledeleteconnectioncb)
        m_handledeleteconnectioncb(fd);
}

// 处理Connection对象接收到的客户端发送过来的一条完整的数据
void TcpServer::handlemessage(std::shared_ptr<Connection> pConn, Buffer* buffer)
{
    if (m_handlemessage)
        m_handlemessage(pConn, buffer);
}

// 当Connection输出缓冲区里的数据已经全部发送出去后，调用该函数，处理之后的逻辑
void TcpServer::sendcomplete(std::shared_ptr<Connection> pConn)
{
    // std::cout << "send complete" << std::endl;

    if (m_handlesendcomplete)
        m_handlesendcomplete(pConn);
}

// 处理事件循环检测中发生超时的情况
void TcpServer::eventlooptimeout(EventLoop *peloop)
{
    if (m_handleeventlooptimeout)
        m_handleeventlooptimeout(peloop);
}

void TcpServer::sethandlecreateconnectioncb(std::function<void(const std::shared_ptr<Socket>)> func)
{
    m_handlecreateconnectioncb = func;
}

void TcpServer::sethandledeleteconnectioncb(std::function<void(int)> func)
{
    m_handledeleteconnectioncb = func;
}

void TcpServer::sethandlemessage(std::function<void(std::shared_ptr<Connection>, Buffer* buffer)> func)
{
    m_handlemessage = func;
}

void TcpServer::sethandlesendcomplete(std::function<void(std::shared_ptr<Connection>)> func)
{
    m_handlesendcomplete = func;
}

void TcpServer::sethandleeventlooptimeout(std::function<void(EventLoop *)> func)
{
    m_handleeventlooptimeout = func;
}

// 从m_clientConnectionMap移除超时的Connection连接，由EventLoop对象通过回调的方式调用
void TcpServer::removeTimeOutConnection(int fd)
{
    {
        // 操作 m_clientConnectionMap 需要加锁
        std::lock_guard<std::mutex> lock(m_mtx);
        m_clientConnectionMap.erase(fd);
    }
}