#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip, uint16_t port, uint16_t subthreads, uint16_t workthreads)
    : m_tcpserver(ip, port, subthreads), 
      m_workthreads(workthreads, "WORK")
{
    m_tcpserver.sethandlecreateconnectioncb([this](std::shared_ptr<Socket> pClientSocket)
                                            { HandleNewConnection(pClientSocket); });

    m_tcpserver.sethandledeleteconnectioncb([this](int fd)
                                            { HandleDeleteConnection(fd); });

    m_tcpserver.sethandleeventlooptimeout([this](EventLoop *peloop)
                                          { HandleEventLoopTimeout(peloop); });

    m_tcpserver.sethandlemessage([this](std::shared_ptr<Connection> pConn, Buffer* buffer)
                                 { HandleOnMessage(pConn, buffer); });

    m_tcpserver.sethandlesendcomplete([this](std::shared_ptr<Connection> pConn)
                                      { HandleSendComplete(pConn); });
}

EchoServer::~EchoServer()
{
}

void EchoServer::Start()
{
    m_tcpserver.start();
}

void EchoServer::Stop()
{
    // 关闭工作线程
    m_workthreads.stop();

    // 关闭I/O线程（主、从事件循环）
    m_tcpserver.stop();
}

// 处理客户端发送过来的消息
void EchoServer::HandleOnMessage(std::shared_ptr<Connection> pConn, Buffer* buffer)
{
    while (buffer->readableBytes() > 4)
    {
        int32_t msgLen = buffer->peekInt32(); // 查看数据长度前缀
        if (buffer->readableBytes() >= 4 + msgLen) // buffer里面有一条完整的TCP数据
        {
            buffer->retrieve(4); // 消耗4字节的前缀
            std::string msg(buffer->retrieveAsString(msgLen)); // 获取数据内容

            // 处理数据/////////////////
            if (m_workthreads.size() != 0) // 如果有工作线程，将数据处理的操作交给工作线程
            {
                m_workthreads.AddTask([this, pConn, msg](){
                    this->OnMessage(pConn, msg);
                });
            }
            else // 如果没有工作线程，数据的处理由当前运行从事件循环的I/O线程执行
            {
                OnMessage(pConn, msg);
            }
        }
        else if (msgLen >= 64 * 1024 * 1024) // 防止炸弹
        {
            pConn->closeconnection(); // 关闭这条链接
        }
        else // buffer 不是一条完整的数据
        {
            break;
        }

        // std::cout << "处理客户端发送过来的数据的是线程：" << syscall(SYS_gettid) << std::endl;

        // std::stringstream oss;
        // oss << "[" << TimesTamp::now().tostring() << "]" << " recv(fd=" << pConn->fd()
        //     << ")" << msg;
        // std::cout << oss.str() << std::endl;
    }
}

// 创建新的客户端连接
void EchoServer::HandleNewConnection(std::shared_ptr<Socket> pClientSocket)
{
    // std::cout << "处理客户端连接请求的是线程：" << syscall(SYS_gettid) << std::endl;
    // std::cout << "connect success" << std::endl;
}

// 处理客户端断开连接的请求
void EchoServer::HandleDeleteConnection(int fd)
{
    // std::cout << "处理客户端断开连接的是线程：" << syscall(SYS_gettid) << std::endl;
    // std::cout << "disconnected success" << std::endl;
}

// 处理服务器将消息发送给客户端之后的业务逻辑
void EchoServer::HandleSendComplete(std::shared_ptr<Connection> pConn)
{
}

// 处理事件循环检测中发生超时的情况
void EchoServer::HandleEventLoopTimeout(EventLoop *peloop)
{
}

// 处理具体业务
void EchoServer::OnMessage(std::shared_ptr<Connection> pConn, std::string msg)
{
    // std::cout << "处理具体业务的是线程：" << syscall(SYS_gettid) << std::endl;
    // 处理客户端发送来的每一条数据
    msg = "reply: " + msg;

    uint32_t msgLen = htonl(msg.size());
    msg.insert(0, reinterpret_cast<char *>(&msgLen), sizeof(msgLen));

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    // 将处理完的数据发送回客户端
    pConn->send(msg);
}