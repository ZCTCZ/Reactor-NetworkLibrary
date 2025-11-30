#pragma once
#include "Log.h"

#include <sys/timerfd.h>
#include <chrono>
#include <unistd.h>

class Timer
{
public:
    Timer(std::chrono::nanoseconds first = std::chrono::seconds(7),
          std::chrono::nanoseconds interval = std::chrono::seconds(7)); // 默认每次闹钟定时时间都是 7
    ~Timer();

    // 返回定时器对象的m_timerfd
    int fd() const;

    // 读取timerfd的数据
    void wait();

private:
    int m_timerfd;
    // 将 ns 转换成 struct itimerspec
    static timespec to_timespec(std::chrono::nanoseconds ns);
};