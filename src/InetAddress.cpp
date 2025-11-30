#include "InetAddress.h"

InetAddress::InetAddress(const string &ip, uint16_t port)
{
    m_sockadd.sin_family = AF_INET;
    inet_pton(AF_INET, ip.data(), &m_sockadd.sin_addr.s_addr);
    m_sockadd.sin_port = htons(port);
}

InetAddress::InetAddress(const struct sockaddr_in& addr)
: m_sockadd(addr) {}



// 返回端口号
uint16_t InetAddress::port() const
{
    return ntohs(m_sockadd.sin_port);
}

// 返回IP
string InetAddress::ip() const
{
    char ipstr[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &m_sockadd.sin_addr.s_addr, ipstr, sizeof(ipstr));
    return ipstr;
}

// 返回指向m_sockadd的指针，指针类型为sockaddr*
const struct sockaddr* InetAddress::addr() const
{
    return (sockaddr*)&m_sockadd;
}

// 返回指向m_sockadd的指针，指针类型为 const sockaddr*
struct sockaddr* InetAddress::addr()
{
    return (sockaddr*)&m_sockadd;
}