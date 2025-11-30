#include "Acceptor.h"

Acceptor::Acceptor(EventLoop *pLoop, const std::string &ip, uint16_t port)
    : m_ploop(pLoop)
{
    // 创建监听套接字对象
    m_psocket = std::shared_ptr<Socket>(Socket::getlistenfd());

    // 设置监听套接字的属性
    // 旧进程处于time_wait状态占据端口的时候，允许新启动的进程可以使用这个端口
    m_psocket->setreuseaddropt();

    // 禁用 Nagle 算法, 降低延迟
    m_psocket->setnodelayopt();

    // 允许多个进程或线程 同时绑定到同一个 IP 和端口
    m_psocket->setreuseportopt();

    // 启用 TCP 的保活机制，在系统层面设置的心跳检测
    m_psocket->setkeepaliveopt();

    // 设置服务器端的IP和端口
    InetAddress servaddr(ip, port);

    // 绑定服务器的IP和端口号
    m_psocket->bind(servaddr);

    // 设置监听
    m_psocket->listen(128);

    // 创建监听套接字对象所对应的Channel对象，负责和Epoll对象交互
    m_pchannel = std::make_shared<Channel>(m_ploop, m_psocket);

    // 设置 和监听套接字关联的Channel对象 的读事件回调函数，当有新的连接建立请求时，将调用这里设置的回调函数
    m_pchannel->setreadeventcb([this]()
                               { onconnect(); });

    // 设置监听套接字的监听事件，并且加入到epoll中
    m_pchannel->enablereading();
}

// 处理客户端建立新连接请求的回调函数
void Acceptor::onconnect()
{
    while (true)
    {
        InetAddress clientaddr; // 记录客户端IP和端口信息的对象

        int clientFd = m_psocket->accept4(clientaddr); // 从内核的Accept队列里面取出已经完成三次握手的连接
        if (clientFd != -1)
        {
            // 使用智能指针管理客户端通信的套接字对象
            auto pClientSocket = std::make_shared<Socket>(clientFd, clientaddr);

            if (pClientSocket->setnodelayopt() == -1)
            {
                // shared_ptr 会在 pClientSocket 离开作用域时自动释放内存，无需手动 delete
                continue;
            }

            // 调用回调函数，创建Connection对象
            m_onconnectcb(pClientSocket);

            std::ostringstream oss;
            oss << "ip=" << clientaddr.ip()
                << ",port=" << clientaddr.port()
                << ",fd=" << pClientSocket->fd()
                << " connected";
            LOG(info) << oss.str();
        }
        else // 调用accept函数从内核的Accept队列里取出连接失败
        {
            if (errno == EINTR) // 信号中断打断了accept函数
            {
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK) // 内核里面的Accept队列为空
            {
                break;
            }
            else if (errno == EMFILE || errno == ENFILE) // 文件描述符耗尽
            {
                continue;
            }
            else // 其他预期之外的错误
            {
                LOG(error) << errno;
                break;
            }
        }
    }
}

Acceptor::~Acceptor() = default;

// 设置m_onconnectcb
void Acceptor::setonconnectcb(std::function<void(std::shared_ptr<Socket>)> fn)
{
    m_onconnectcb = fn;
}