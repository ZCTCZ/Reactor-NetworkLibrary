#pragma once

#include <arpa/inet.h>
#include <string>

using std::string;

class InetAddress
{
public:
    InetAddress(const string& ip = string(), uint16_t port = 0);
    InetAddress(const struct sockaddr_in& addr);
    InetAddress(const InetAddress& ) = default;
    ~InetAddress() = default;

    uint16_t port() const; // 返回IP
    string ip() const; // 返回端口号
    const struct sockaddr* addr() const; // 返回指向m_sockadd的指针，指针类型为 const sockaddr*
    struct sockaddr* addr(); // 返回指向m_sockadd的指针，指针类型为 sockaddr*

private:
    struct sockaddr_in m_sockadd;
};