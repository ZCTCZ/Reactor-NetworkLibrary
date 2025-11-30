#include "Log.h"

#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <chrono>

using std::cin;
using std::cout;
using std::endl;

int recvdata(int fd)
{
    char buf[1024] = {'\0'};
    uint32_t len = 0;
    int recvLen = recv(fd, static_cast<void *>(&len), sizeof(len), 0); // 读取消息头，获取消息体长度
    if (len == -1)
    {
        LOG(error);
        perror("recv err");
        return -1;
    }
    else if (len == 0)
    {
        LOG(info) << "服务器已关闭";
        return -2;
    }
    else // 读取消息体
    {
        recvLen = recv(fd, buf, sizeof(buf)-1, 0);
        if (recvLen == -1)
        {
            LOG(error);
            perror("recv err");
            return -1;
        }
        else
        {
            // buf[recvLen] = '\0';
            // std::cout << buf << std::endl;
            memset(buf, 0, sizeof(buf));
            return 0;
        }
    }   
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::string errMsg = "usage:" + std::string(argv[0]) + " IP Port";
        LOG(error) << errMsg;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        LOG(error) << "socket() err";
        return -1;
    }

    struct sockaddr_in servaddr;
    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr.s_addr) == -1)
    {
        LOG(error) << "inet_pton() err";
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        LOG(error) << "connect() err";
        return -1;
    }
    else
    {
        cout << "connect success" << endl;
    }

    // sleep(15);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) // 向服务端发送数据
    {
        std::string msg;
        msg += "这是第 ";
        msg += std::to_string(i);
        msg += " 条消息";

        uint32_t len = htonl(msg.length());
        msg.insert(0, reinterpret_cast<char *>(&len), sizeof(len)); // 将消息体的长度插入每条消息的首部

        if (send(fd, msg.data(), msg.size(), 0) == -1)
        {
            LOG(error) << "send() err";
            break;
        }

        // 接收服务器返回的数据
        if (recvdata(fd) != 0)
        {
            break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    cout << "Execution time: " << elapsed.count() << " s" << endl;

    close(fd);
    return 0;
}