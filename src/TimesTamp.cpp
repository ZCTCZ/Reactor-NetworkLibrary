#include "TimesTamp.h"

TimesTamp::TimesTamp()
    : m_secSinceEpoch(time(nullptr))
{
}

TimesTamp::TimesTamp(time_t secSinceEpoch)
    : m_secSinceEpoch(secSinceEpoch)
{
}

TimesTamp TimesTamp::now()
{
    return TimesTamp();
}

time_t TimesTamp::toint() const // 返回时间戳对象表示的整数形式的时间（从Epoch到now的秒数）
{
    return m_secSinceEpoch;
}

std::string TimesTamp::tostring() const // 返回时间戳对象表示的字符串形式的时间（yyyy-mm-dd HH:MM::SS）
{
    tm *t = localtime(&m_secSinceEpoch);
    char buf[128] = {0};
    // 使用 snprintf 进行格式化，它会自动处理+1900和+1，并且能方便地控制格式
    snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900,
             t->tm_mon + 1,
             t->tm_mday,
             t->tm_hour,
             t->tm_min,
             t->tm_sec);

    return buf;
}

std::string TimesTamp::tostringData() const // 返回时间戳对象表示的字符串形式的时间（yyyy-mm-dd）
{
    tm *t = localtime(&m_secSinceEpoch);
    char buf[128] = {0};
    // 使用 snprintf 进行格式化，它会自动处理+1900和+1，并且能方便地控制格式
    snprintf(buf, sizeof(buf), "%4d-%02d-%02d",
             t->tm_year + 1900,
             t->tm_mon + 1,
             t->tm_mday);
             
    return buf;
}


/*
#include <iostream>
#include <unistd.h>
int main()
{
    TimesTamp tt;
    std::cout << tt.toint() << std::endl;
    std::cout << tt.tostring() << std::endl;

    sleep(1);

    std::cout << tt.now().toint() << std::endl;
    std::cout << tt.now().tostring() << std::endl;

    return 0;
}
*/