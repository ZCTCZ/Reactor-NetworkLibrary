#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>

class Log
{
public:
    enum Level
    {
        debug,
        info,
        warn,
        error
    };

    enum OutputTarget 
    {
        CONSOLE,    // 输出到控制台
        FILE        // 输出到文件
    };

    Log(Level lv, const char *file, int line, const char *func);
    Log(const Log&) = delete;
    Log(const Log&&) = delete;
    Log& operator()(const Log&) = delete;
    Log& operator()(const Log&&) = delete;

    ~Log(); // 触发析构函数的时候才输出日志内容

    // 重载operator<<
    template <class T>
    Log &operator<<(const T &t)
    {
        os_ << t;
        return *this;
    }

    // 设置输出目标
    static void SetOutputTarget(OutputTarget target, const std::string& filename = "");

    // 获取当前输出流
    static std::ostream& GetOutputStream();

private:
    static const char *level2str(Level l);
    std::ostringstream os_;
    Level lv_;

    static OutputTarget output_target_; // 静态成员变量，表示日志输出形式
    static std::ofstream log_file_; // 静态成员变量，表示记录日志的文件输出流
    static std::string output_file_name_; // 静态成员变量，表示记录日志的文件的名称

    std::mutex m_mtx;
};

/* 
    真正被用户用的宏 
    __FILE__: 当前源文件名
    __LINE__: 当前行号
    __func__: 当前函数名
    lv:debug、info、warn、error。
*/
#define LOG(lv) Log(Log::lv, __FILE__, __LINE__, __func__)