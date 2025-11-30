#pragma once

#include "InetAddress.h"
#include "Log.h"

class Socket
{
public:
    Socket(int fd = 0);
    Socket(int fd, InetAddress clientaddr);
    ~Socket();

    // 获取通信套接字
    int fd() const;

    // 设置套接字的 SO_REUSEADDR 属性
    int setreuseaddropt();

    // 设置套接字的 TCP_NODELAY 属性
    int setnodelayopt();

    // 设置套接字的 SO_REUSEPORT 属性
    int setreuseportopt();

    // 设置套接字的 SO_KEEPALIVE 属性
    int setkeepaliveopt();

    // 静态工厂方法，返回监听套接字对象
    static Socket* getlistenfd();

    // 绑定IP和端口
    int bind(const InetAddress& servaddr);

    // 设置监听
    int listen(int n = 128);

    // 接收客户端连接请求，生成和客户端通信的套接字
    int accept4(InetAddress& clientaddr); // 生成的通信套接字为非阻塞的

    // 获取ip
    std::string getip() const;

    // 获取端口
    uint16_t getport() const;

private:
    int m_fd;
    std::string m_ip;
    uint16_t m_port;
};