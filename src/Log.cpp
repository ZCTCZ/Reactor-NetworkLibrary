#include "Log.h"
#include "AsyncLogging.h"
#include "TimesTamp.h"

#include <unistd.h>
#include <sys/syscall.h>

// 初始化静态成员变量
Log::OutputTarget Log::output_target_ = Log::CONSOLE;
std::ofstream Log::log_file_;
std::string Log::output_file_name_;

/*
    %Y: 四位数年份（如 2024）
    %m: 两位数月份（01-12）
    %d: 两位数日期（01-31）
    %H: 24小时制小时（00-23）
    %M: 分钟（00-59）
    %S: 秒数（00-59）
*/
Log::Log(Level lv, const char *file, int line, const char *func)
    : lv_(lv)
{
    auto now = std::chrono::system_clock::now(); // 获取当前时间点
    std::time_t t = std::chrono::system_clock::to_time_t(now); // 转成time_t格式（/* Seconds since the Epoch.  */）
    std::tm tm = *std::localtime(&t); // 转成本地时间结构
    os_ << '[' << level2str(lv) << ']'
        << "[PID:" << syscall(SYS_gettid) << ']' // 线程ID
        << '[' << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << ']'
        << '[' << file << ':' << line << ']'
        << '[' << func << "] ";
}

// 触发析构函数的时候才输出日志内容
Log::~Log() 
{
    if (AsyncLogging::exist) // 如果定义了 AsyncLogging 类的单例对象，就可以使用异步日志输出
    {
        AsyncLogging* asyncLog = AsyncLogging::getInstance();
        asyncLog->setOutput(&GetOutputStream());// 设置日志异步输出的目的地
        auto msg = std::move(os_.str() + '\n');
        // std::cout << msg << std::endl;
        asyncLog->Append(msg.data(), msg.size()); // 向日志系统前端缓冲区写入日志
    }
    else //使用普通的同步日志输出 
    {
        std::lock_guard<std::mutex> lock(m_mtx); // GetOutputStream()的返回值是临界区，访问需要加锁

        // std::string msg = std::move(os_.str() + '\n');
        // std::cout << os_.str() << std::endl;
        // GetOutputStream().write(msg.data(), msg.size());

        GetOutputStream() << os_.str() << '\n';
        // GetOutputStream().flush();
    }
    
} //std::cerr 默认无缓冲，能保证日志及时输出，避免因程序崩溃导致日志丢失

const char *Log::level2str(Level l)
{
    switch (l)
    {
    case debug:
        return "DEBUG";
    case info:
        return "INFO ";
    case warn:
        return "WARN ";
    case error:
        return "ERROR";
    }
    return "UNKN ";
}

// 设置输出目标
void Log::SetOutputTarget(OutputTarget target, const std::string& filename)
{
    if (output_target_ != target || (output_target_ == target && target == FILE && output_file_name_ != filename))
    {
        output_target_ = target;
        if (target == FILE && !filename.empty())
        {
            output_file_name_ = filename;
            auto file = TimesTamp::now().tostringData(); // 将当前系统时间作为前缀
            file += "_";
            file += filename;

            log_file_.open(file, std::ios::app); // 以追加的形式打开文件
            
            if (!log_file_.is_open()) // 打开文件失败
            {
                std::cerr << "open " << filename << " failed" << std::endl;
                output_target_ = CONSOLE; // 日志输出到控制台上
            }
        }
        else if (target == CONSOLE && log_file_.is_open()) // 关闭已经打开的文件
        {
            log_file_.close();
        }
    }
}

// 获取当前输出流
std::ostream& Log::GetOutputStream()
{
    if (output_target_ == FILE && log_file_.is_open())
    {
        return log_file_;
    }

    return std::cerr;
}
