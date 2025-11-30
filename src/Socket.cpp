#include "Socket.h"

#include <unistd.h>
#include <netinet/tcp.h>

Socket::Socket(int fd)
    : m_fd(fd)
{
}

Socket::Socket(int fd, InetAddress clientaddr)
    : m_fd(fd), m_ip(clientaddr.ip()), m_port(clientaddr.port())
{
}

Socket::~Socket()
{
    ::close(m_fd);
}

// 获取通信套接字
int Socket::fd() const
{
    return m_fd;
}

// 设置套接字的 SO_REUSEADDR 属性
int Socket::setreuseaddropt()
{
    // 设置监听套接字的属性
    int opt = 1;
    // 使得服务器重启后，原来的服务器程序还处于time-wait状态，可以复用同一个端口号
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        LOG(error) << "setsockopt(SO_REUSEADDR) err";
        return -1;
    }
    return 0;
}

// 设置套接字的 TCP_NODELAY 属性
int Socket::setnodelayopt()
{
    int opt = 1;
    // 禁用 Nagle 算法, 降低延迟
    if (setsockopt(m_fd, SOL_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1)
    {
        LOG(error) << "setsockopt(TCP_NODELAY) err";
        return -1;
    }
    return 0;
}

// 设置套接字的 SO_REUSEPORT 属性
int Socket::setreuseportopt()
{
    int opt = 1;
    // 允许多个进程或线程 同时绑定到同一个 IP 和端口
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
    {
        LOG(error) << "setsockopt(SO_REUSEPORT) err";
        return -1;
    }
    return 0;
}

// 设置套接字的 SO_KEEPALIVE 属性
int Socket::setkeepaliveopt()
{
    int opt = 1;
    // 启用 TCP 的保活机制，在系统层面设置的心跳检测
    if (setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) == -1)
    {
        LOG(error) << "setsockopt(SO_KEEPALIVE) err";
        return -1;
    }
    return 0;
}

// 静态工厂方法，返回监听套接字对象
Socket *Socket::getlistenfd()
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd >= 0)
    {
        auto pSocket = new Socket(fd);
        return pSocket;
    }
    else
    {
        LOG(error) << "socket() error";
        return nullptr;
    }
}

// 绑定IP和端口
int Socket::bind(const InetAddress &servaddr)
{
    int ret = ::bind(m_fd, servaddr.addr(), sizeof(struct sockaddr));
    if (ret == -1)
    {
        LOG(error) << "bind() err";
    }

    m_ip = servaddr.ip();
    m_port = servaddr.port();

    return ret;
}

// 设置监听
int Socket::listen(int n)
{
    int ret = ::listen(m_fd, n);
    if (ret == -1)
    {
        LOG(error) << "listen() err";
    }

    return ret;
}

// 接收客户端连接请求，生成和客户端通信的套接字
int Socket::accept4(InetAddress &clientaddr) // 生成的通信套接字为非阻塞的
{
    socklen_t clientaddrLen = sizeof(struct sockaddr);
    int ret = ::accept4(m_fd, clientaddr.addr(), &clientaddrLen, SOCK_NONBLOCK);
    if (ret == -1)
    {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK && errno != EMFILE && errno != ENFILE)
        {
            LOG(error) << "accept4() err";
        }
    }
    return ret;
}

// 获取ip
std::string Socket::getip() const
{
    return m_ip;
}

// 获取端口
uint16_t Socket::getport() const
{
    return m_port;
}