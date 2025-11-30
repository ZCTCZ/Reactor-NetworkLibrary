#include "AsyncLogging.h"
#include "TimesTamp.h"
#include <fstream>

bool AsyncLogging::exist = false;

AsyncLogging::AsyncLogging(uint32_t flushInterval)
    : m_output(&std::cerr),
      m_flushInterval(flushInterval),
      m_currentBuffer(new LogBuffer),
      m_nextBuffer(new LogBuffer),
      m_running(false),
      m_thread([this](){
        this->ThreadFunc();
      })
{
    m_nextBuffer->reset();
    m_buffers.reserve(16); // 避免后端线程在 频繁 push_back 时触发 vector 重新分配和拷贝
}

AsyncLogging::~AsyncLogging() 
{
    if (m_running)
    {
        stop();
    }

    // 判断 m_output 是不是文件输出流
    std::ofstream* file = dynamic_cast<std::ofstream*>(m_output);
    if (file)
    {
        file->close();
    }
}

void AsyncLogging::start() // 启动异步日志
{
    m_running.store(true);
}

void AsyncLogging::stop() // 关闭异步日志
{
    m_running.store(false);
    m_cond.notify_one();
    m_thread.join();
}

void AsyncLogging::Append(const char *logData, uint32_t len)
{
    /////////////////////////////锁区域/////////////////////////////////////////
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_currentBuffer->available() >= len) // 当前缓冲区可以放下 len 长度的日志
        {
            m_currentBuffer->append(logData, len);
        }
        else // 当前缓冲区放不下 len 长度的日志
        {
            // 高性能
            m_buffers.push_back(std::move(m_currentBuffer)); // 将当前缓冲区里的数据移动到缓冲队列里

            if (m_nextBuffer) // 备胎缓冲区可用
            {
                m_currentBuffer = std::move(m_nextBuffer); // 夺舍备胎缓冲区
            }
            else // 备胎缓冲区已被夺舍，现在不可用。（这种情况很少发生，除非日志写得太快）
            {
                m_currentBuffer.reset(new LogBuffer); // 再生成一块Buffer，此时日志系统的前端就有三个Buffer
            }

            m_currentBuffer->append(logData, len); // 存放日志
            m_cond.notify_one();                   // 唤醒后端负责日志落盘的线程
        }
    }
    /////////////////////////////锁区域/////////////////////////////////////////
}

// 处理日志的落盘任务，由单独的日志线程来执行
void AsyncLogging::ThreadFunc()
{
    std::vector<BufferPtr> buffersToWrite; // 用来替换 m_buffer
    buffersToWrite.reserve(16);

    BufferPtr newBuffer1(new LogBuffer);
    BufferPtr newBuffer2(new LogBuffer);

    while (m_running || m_currentBuffer->cur > 0)
    {
        /////////////////////////////锁区域/////////////////////////////////////////
        {
            std::unique_lock<std::mutex> lock(m_mtx);

            if (m_buffers.empty()) // 如果前端缓存队列为空，则睡眠 flushInterval 秒
            {
                m_cond.wait_for(lock, std::chrono::seconds(m_flushInterval));
            }

            // 醒来之后，发现m_currentBuffer所指的空间 以及 前端缓冲队列都是空的，则continue
            if(m_buffers.empty() && m_currentBuffer->cur == 0)
            {
                continue;
            }

            // 线程超时醒来后，或者被其他线程唤醒。此时日志线程已经拿到锁，其他的线程在这个期间无法写日志。
            m_buffers.push_back(std::move(m_currentBuffer)); // 将当前缓冲区m_currentBuffer里的内容添加到缓冲队列m_buffers里
            m_currentBuffer = std::move(newBuffer1); // 用 newBuffer1 缓冲区替换 m_currentBuffer，保证日志系统的前端不会停

            buffersToWrite.swap(m_buffers); // 交换日志系统前后端的缓冲队列，同样是为了保证日志系统的前端不会停

            if (!m_nextBuffer) // 如果当前缓冲区的备胎已经被夺舍，需要给它恢复，以便下次可以继续被夺舍
            {
                m_nextBuffer = std::move(newBuffer2);
            }
        }
        /////////////////////////////锁区域/////////////////////////////////////////

        if (buffersToWrite.size() > 25) // 如果日志系统后端的缓冲队列 bufferToWrite 里面日志块的数量大于25，则丢弃多余的日志块，只剩下两块。
        {
            char msg[256] = {0};
            snprintf(msg, sizeof(msg), "Dropped log messages at %s, %zd larger buffers\n",
                    TimesTamp::now().tostring().data(), buffersToWrite.size()-2);

            // 将msg落盘到日志里面
            ////////////////////
            *m_output << msg;
            m_output->flush();
            fputs(msg, stderr);
            
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end()); // 只留下最前面的两块
        }

        // 将后端的缓冲队列里的内容全部落盘
        for (const auto& e : buffersToWrite)
        {
            ////////////////////
            *m_output << e->data;
            m_output->flush();
        }


        if (buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2); // 清除多余的数据，只剩下两个后续给newBuffer1 和 newBuffer2 赋值。
        }

        // 如果 newBuffer1 所指的空间已经和 m_currentBuffer 交换，需要将newBuffer1重新赋值。
        if (!newBuffer1)
        {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset(); // 将 LogBuffer 格式化
        }

        // 如果 newBuffer2 所指的空间已经和 m_nextBuffer 交换，需要将newBuffer2重新赋值。
        if (!newBuffer2)
        {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
    }
}

void AsyncLogging::setOutput(std::ostream* os)
{
    m_output = os;
}

// 获取 AsyncLogging 类的单例
AsyncLogging* AsyncLogging::getInstance(uint32_t flushInterval)
{
    if (!exist)
    {
        exist = true;
    }

    static AsyncLogging asyncLog(flushInterval);
    return &asyncLog;
}


