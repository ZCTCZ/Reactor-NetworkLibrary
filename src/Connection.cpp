#include "Connection.h"
#include "Socket.h"
#include "Channel.h"

#include <cstring>

Connection::Connection(std::shared_ptr<Socket> psocket, EventLoop *ploop)
    : m_psocket(psocket),
      m_ploop(ploop),
      m_pchannel(std::make_shared<Channel>(ploop, m_psocket)),
      m_disconnect(false)
{
    // 设置 和通信套接字关联的Channel对象 的读事件回调函数
    m_pchannel->setreadeventcb([this]()
                               { this->onmessage(); });

    // 设置 关闭TCP连接时，析构该条TCP连接通信套接字所属的Connection对象 的回调函数
    m_pchannel->setcloseconnectioncb([this]()
                                     { closeconnection(); });

    // 设置 和通信套接字关联的Channel对象 的写事件回调函数
    m_pchannel->setwriteeventcb([this]()
                                { this->sendto(); });

    // 设置边沿模式
    m_pchannel->setepollet();
}

// 在Connection对象析构的时候，智能指针会自动释放其管理的内存，无需手动delete
Connection::~Connection()
{
    m_pchannel->remove(); // 移除事件循环检测

    std::ostringstream oss;
    oss << "ip=" << m_psocket->getip()
        << ",port=" << m_psocket->getport()
        << ",fd=" << m_psocket->fd();

    if (m_istimeout)
    {
        oss << " TimeOut";
    }
    else
    {
        oss << " disconnected";
    }

    LOG(info) << oss.str();
}

// 获取 通信套接字fd
int Connection::fd() const
{
    return m_psocket->fd();
}

// 将Connection连接添加到延迟删除树
void Connection::closeconnection()
{
    m_disconnect.store(true);
    m_ploop->delayDelete(fd());
}

// 设置 m_handlemessage
void Connection::sethandlemessage(std::function<void(std::shared_ptr<Connection>, Buffer *)> func)
{
    m_handlemessagecb = func;
}

// 设置 m_sendcompletecb
void Connection::setsendcomplete(std::function<void(std::shared_ptr<Connection>)> func)
{
    m_sendcompletecb = func;
}

// 处理客户端旧连接的I/O
void Connection::onmessage()
{
    // 更新Connection对象的时间戳
    m_lasttime = TimesTamp::now();
    m_ploop->updateConnection(fd());

    // 一次性将通信套接字的读缓冲区读空
    while (true)
    {
        int errnum = 0;
        ssize_t recvLen = m_inputbuf.readFd(m_psocket->fd(), &errnum);

        if (recvLen > 0)
        {
            continue;
        }
        else if (recvLen == 0) // 对端正常关闭
        {
            if (0 == m_inputbuf.readableBytes()) // Connection 的缓冲区中没有数据需要处理
            {
                closeconnection();
                return;
            }
            else // Connection 的缓冲区中还有数据需要处理
            {
                break;
            }
        }
        else if (recvLen == -1)
        {
            if (errnum == EAGAIN || errnum == EWOULDBLOCK) // 缓冲区里以的数据已经读完
            {
                break;
            }
            else if (errnum == EINTR) // 由于信号中断，打断了recv读取缓冲区数据的过程
            {
                continue; // 重新开始读取
            }
            else if (errnum == ECONNRESET) // 对端异常关闭
            {
                LOG(warn) << "peer reset, fd=" << fd();
                closeconnection();
                return;
            }
            else // recv函数确实发生了预期之外的错误, 服务器端主动和发生故障的客户端断开连接
            {
                LOG(error) << "recv() err" << errno;
                closeconnection();
                return;
            }
        }
    }

    // 调用回调函数，处理客户端发送来的每一条数据
    m_handlemessagecb(shared_from_this(), &m_inputbuf);

    // while (true) // 解析客户端发送过来的每一条数据
    // {
    //     // 解析数据
    //     if (m_inputbuf.readableBytes() < sizeof(uint32_t))// m_inputbuf连4个字节都没有，说明连消息头都接收不全
    //     {
    //         break;
    //     }

    //     uint32_t len = 0;
    //     memcpy(static_cast<void *>(&len), m_inputbuf.data(), sizeof(len));
    //     len = ntohl(len);
    //     if (m_inputbuf.size() < len + sizeof(len)) // m_inputbuf 里面不是一条完整的数据，暂时存放在读缓冲区内，等下次客户端发送余下的数据。
    //     {
    //         break;
    //     }

    //     std::string msg(m_inputbuf.data() + 4, len); // 从 m_inputbuf 里面获取一条完整消息的消息体（拷贝构造）
    //     m_inputbuf.erase(0, len + sizeof(len));      // 将已经取出的消息从 m_inputbuf 中删除

    //     // 调用回调函数，处理客户端发送来的每一条数据
    //     m_handlemessagecb(shared_from_this(), msg);
    // }
}

// 将数据放到Connection对象的写缓冲区里，并注册写事件
void Connection::send(const std::string &msg)
{
    if (!m_disconnect.load())
    {
        if (m_ploop->isEventLoopThread()) // 如果执行当前函数的是I/O线程，则直接将数据写到Connection对象的写缓冲区中
        {
            writeTo(msg);
        }
        else // 如果执行当前函数的是工作线程，则将发送数据的操作交给I/O线程来做。不能在工作线程内发送数据，会和I/O线程产生竞态条件
        {
            // 将发送数据的任务添加到事件循环对象的任务队列中，等待I/O线程来处理任务
            m_ploop->addTask([this, msg]() // msg在这里是值拷贝
                             { writeTo(msg); });
        }
    }
}

// 将 Connection 写缓冲区里的数据发送到内核的写缓冲区
void Connection::sendto()
{
    if (!m_disconnect.load())
    {
        while (m_outputbuf.readableBytes() > 0)
        {
            // 加上 MSG_NOSIGNAL 信号，当对端已关闭(RST)时，内核不会给进程发送SIGPIPE信号，只会有errno = EPIPE 错误码
            ssize_t writeLen = ::send(fd(), m_outputbuf.peek(), m_outputbuf.readableBytes(), MSG_NOSIGNAL);
            if (writeLen > 0)
            {
                m_outputbuf.retrieve(writeLen);
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) // 内核的发送缓冲区已满
                {
                    break;
                }
                else if (errno == EPIPE || errno == ECONNRESET)
                {
                    LOG(warn) << "peer closed, fd=" << fd();
                    closeconnection();
                    break;
                }
                else
                {
                    // 其他致命错误
                    LOG(error) << "send() fatal, errno=" << errno;
                    closeconnection();
                    break;
                }
            }
        }

        // m_outputbuf里面所有的数据都发送完，则停止监听读事件
        if (0 == m_outputbuf.readableBytes())
        {
            m_pchannel->disablewriting();
            m_sendcompletecb(shared_from_this());
        }
    }
}

// 将Connection对象的Channel 添加到事件循环中，让epoll监听它的读事件
void Connection::addToEpoll()
{
    m_pchannel->enablereading();
}

// 将待发送的数据msg写入Connection对象的写缓冲区
void Connection::writeTo(const std::string &msg)
{
    m_outputbuf.append(msg.data(), msg.size());
    m_pchannel->enablewriting(); // 注册写事件
}

// 判断当前连接是否超时
bool Connection::isTimeOut(time_t interval)
{
    m_istimeout = time(nullptr) - m_lasttime.toint() >= interval;
    return m_istimeout;
}