#pragma once

#include <ctime>
#include <string>

class TimesTamp
{
public:
    TimesTamp();
    TimesTamp(time_t secSinceEpoch);

    static TimesTamp now(); // 返回当前时间的时间戳对象

    time_t toint() const; // 返回时间戳对象表示的整数形式的时间（从Epoch到now的秒数）

    std::string tostring() const; //返回时间戳对象表示的字符串形式的时间（yyyy-mm-dd HH:MM::SS）
    
    std::string tostringData() const; // 返回时间戳对象表示的字符串形式的时间（yyyy-mm-dd）
private:
    time_t m_secSinceEpoch;
};