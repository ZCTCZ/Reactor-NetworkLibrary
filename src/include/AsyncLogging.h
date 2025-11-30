#pragma once

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <cassert>
#include <iostream>

struct LogBuffer // 辅助类，表示日志缓冲区
{
    char data[4 * 1000 * 1000];
    uint32_t cur;
    LogBuffer()
    {
        reset();
    }
    void reset()
    {
        memset(data, 0, sizeof(data));
        cur = 0;
    }

    size_t available() const { return sizeof(data) - cur; }

    void append(const char* logData, int len)
    {
        assert(available() >= len);
        memcpy(data+cur, logData, len);
        cur += len;
    }
};

class AsyncLogging
{
public:
    using BufferPtr = std::unique_ptr<LogBuffer>;

    static AsyncLogging* getInstance(uint32_t flushInterval = 3);

    ~AsyncLogging();

    void start(); // 启动异步日志

    void stop(); // 关闭异步日志

    void Append(const char *logData, uint32_t len); // 记录日志

    void ThreadFunc(); // 日志线程运行的函数，负责日志落盘

    void setOutput(std::ostream* os);// 设置日志的输出方式，磁盘输出还是终端控制台输出

    static bool exist; // 标志是否存在 AsyncLogging 类的单例对象

private:
    AsyncLogging(uint32_t flushInterval);
    AsyncLogging(const AsyncLogging&) = delete;
    AsyncLogging(const AsyncLogging&&) = delete;

    AsyncLogging& operator()(const AsyncLogging&) = delete;
    AsyncLogging& operator()(const AsyncLogging&&) = delete;


    BufferPtr m_currentBuffer;        // 当前正在使用的缓冲区
    BufferPtr m_nextBuffer;           // 备胎缓冲区
    std::vector<BufferPtr> m_buffers; // 缓冲队列，里面存放的是指向Buffer的指针

    uint32_t m_flushInterval; // 刷新时间，默认3s
    std::ostream* m_output;  // 日志最终的输出位置，磁盘或是终端控制台

    std::thread m_thread; // 日志线程
    std::mutex m_mtx;
    std::condition_variable m_cond;
    std::atomic<bool> m_running;
};
