#include "Timer.h"


Timer::Timer(std::chrono::nanoseconds first, std::chrono::nanoseconds interval)
    : m_timerfd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
{
    if (m_timerfd == -1)
    {
        LOG(error) << "timerfd_create() err";
    }

    struct itimerspec timeout = {0};
    timeout.it_value = Timer::to_timespec(first); // 设置首次发生超时的时间
    timeout.it_interval = Timer::to_timespec(interval); // 设置循环超时的时间（不包括首次）

    if (timerfd_settime(m_timerfd, 0, &timeout, nullptr) == -1)
    {
        LOG(error) << "timerfd_settime() err";
    }
}

Timer::~Timer()
{
    ::close(m_timerfd);
}

// 返回定时器对象的m_timerfd
int Timer::fd() const
{
    return m_timerfd;
}

// 读取timerfd的数据（防止闹钟一直响）
void Timer::wait()
{
    uint64_t val;
    read(m_timerfd, &val, sizeof(val));
}

// 将 ns 转换成 struct timespec
timespec Timer::to_timespec(std::chrono::nanoseconds ns)
{
    itimerspec ts{};
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(ns);
    ts.it_value.tv_sec  = sec.count();
    ts.it_value.tv_nsec = (ns - sec).count();
    return ts.it_value;
}