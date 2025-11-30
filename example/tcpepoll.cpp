#include "EchoServer.h"
#include "Log.h"
#include "AsyncLogging.h"

#include <sys/signal.h>
#include <memory>

std::unique_ptr<EchoServer> pechoServer;

// 信号处理函数
void signalhandinging(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        pechoServer->Stop();
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::string errMsg = "usage:" + std::string(argv[0]) + " <IP> <Port>";
        LOG(error) << errMsg;
        return -1;
    }

    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = signalhandinging;
    sigemptyset(&sa.sa_mask);

    // 注册信号处理函数
    if (sigaction(SIGINT, &sa, nullptr) == -1)
    {
        LOG(error) << "sigaction(SIGINT) err";
        return -1;
    }

    // 定义异步日志对象
    AsyncLogging* asyncLog = AsyncLogging::getInstance();
    asyncLog->start(); // 启动异步日志

    // 设置日志输出到指定文件
    Log::SetOutputTarget(Log::FILE, "log");

    pechoServer = std::make_unique<EchoServer>(argv[1], atoi(argv[2]), 4, 0); // 4个子线程，0个工作线程

    // 开启事件循环
    pechoServer->Start();

    return 0;
}