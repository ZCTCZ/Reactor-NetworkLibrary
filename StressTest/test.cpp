// g++ -O2 -std=c++17 test.cpp -o test
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <atomic>

const char *serv_ip = "192.168.198.133";
uint16_t serv_port = 60001;
const int CONN_N = 500;     // 并发连接数
const int PIPE_DEPTH = 500; // 每条连接管道深度

struct Conn
{
    int fd;
    int inflight; // 未完成请求数
    std::vector<char> recv_buf;
    size_t recv_off; // 当前接收缓冲区的偏移量，即已经接收到并且未处理的数据量
    Conn(int sock) : fd(sock), inflight(0), recv_off(0) { recv_buf.resize(4096); }
};

static int epfd;
static std::vector<Conn *> conns;
static uint64_t send_cnt = 0, recv_cnt = 0;
static std::atomic<uint64_t> g_inflight(0);
static std::chrono::steady_clock::time_point start_t;

static void set_nonblock(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static bool write_all(int fd, const char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t w = ::send(fd, buf, n, MSG_NOSIGNAL);
        if (w > 0)
        {
            n -= w;
            buf += w;
            continue;
        }
        if (w == 0)
        {
            return false;
        }
        if (errno == EINTR) // 信号中断
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) // 内核写缓冲区已满
            return false;
        return false;
    }
    return true;
}

static void make_request(std::string &out)
{
    out.clear();
    static int n = 0;
    out += "这是第";
    out += std::to_string(n++);
    out += "条消息";
    uint32_t len = htonl(out.size());
    out.insert(0, (char *)&len, 4);
    // std::cout << ntohl(len) << '\t' << out << std::endl;
}

static void try_fill_pipe(Conn *c)
{
    std::string req;
    while (c->inflight < PIPE_DEPTH)
    {
        make_request(req);
        if (!write_all(c->fd, req.data(), req.size()))
        {
            break;
        }
        c->inflight++;
        g_inflight++; // 同时更新全局计数器
        send_cnt++;
    }
}

static void handle_recv(Conn *c)
{
    // 1. 循环接收数据
    while (true)
    {
        // 2. 调用 recv 从 socket 读取数据
        ssize_t n = recv(c->fd, c->recv_buf.data() + c->recv_off, c->recv_buf.size() - c->recv_off, 0);

        // 3. 处理 recv 的返回值
        if (n < 0)
        { // 出错
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 这不是真正的错误。它表示 socket 的接收缓冲区里已经没数据可读了。
                // 因为我们使用了非阻塞模式（ET模式），所以读完所有数据后就会遇到这个情况。
                // 此时应该退出循环，等待下一次 EPOLLIN 事件。
                break;
            }
            // 其他错误，比如连接被重置等，也退出循环。
            break;
        }
        if (n == 0)
        { // 对端关闭连接
            // recv 返回 0 表示对方已经关闭了连接。
            // 此时也应该退出循环，之后可以做一些清理工作（本示例代码未处理）。
            break;
        }

        // 4. 更新缓冲区偏移量
        // 如果成功读取了 n 字节的数据，就将缓冲区的有效数据长度增加 n。
        c->recv_off += n;

        // 5. 循环解析消息包
        // 只要缓冲区中的数据长度大于等于4字节（一个包头的大小），就尝试解析。
        uint32_t processed = 0; // 已经处理的数据长度
        while (c->recv_off >= 4)
        {
            // 6. 读取包体长度
            // 从缓冲区的最开始读取4个字节，并将其从网络字节序转换为主机字节序，得到包体长度。
            uint32_t body_len = 0;
            memcpy(&body_len, c->recv_buf.data() + processed, sizeof(body_len));
            body_len = ntohl(body_len);

            // 7. 检查是否收到了一个完整的包
            if (c->recv_off >= 4 + body_len)
            {
                // 如果当前缓冲区的数据长度大于等于“包头长度 + 包体长度”，
                // 说明我们至少收到了一个完整的消息包。

                // std::string str(c->recv_buf.data()+4, body_len);
                // std::cout << str << std::endl;

                // 8. 处理完整的包
                // a. 更新缓冲区状态：将有效数据长度减去一个完整包的长度。
                c->recv_off -= (4 + body_len);
                processed += (4 + body_len);
                // b. 更新 inflight 计数：表示一个请求已成功收到响应。
                c->inflight--;
                g_inflight--; // 同时更新全局计数器
                // c. 更新总接收计数。
                recv_cnt++;
                // d. 整理缓冲区：将缓冲区中已经处理过的数据丢弃，把后面的数据（可能是下一个不完整的包）
                //    移动到缓冲区的最前面。这是为了下一次能从头开始解析。
                // memmove(c->recv_buf.data(), c->recv_buf.data() + 4 + body_len, c->recv_off);
            }
            else
            {
                // 如果缓冲区的数据还不够一个完整的包（比如只收到了包头和一部分包体），
                // 那么就退出内层循环，等待下一次 recv 接收更多数据。
                break;
            }
        }

        // 一次性整理缓冲区：将缓冲区中已经处理过的数据丢弃，把最后还剩下的半条数据（也可能没有数据）
        // 移动至缓冲区的最前面。
        memmove(c->recv_buf.data(), c->recv_buf.data()+processed, c->recv_off);
    }
    // 9. 尝试补充发送请求
    // 在处理完一批接收后，管道中可能有了空位（因为 inflight 减少了），
    // 所以调用 try_fill_pipe 尝试发送新的请求，以保持管道深度。
    try_fill_pipe(c);
}

static void create_connections()
{
    epfd = epoll_create1(0);
    for (int i = 0; i < CONN_N; ++i)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        set_nonblock(fd);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serv_port);
        inet_pton(AF_INET, serv_ip, &addr.sin_addr);

        connect(fd, (sockaddr *)&addr, sizeof(addr));
        Conn *c = new Conn(fd);
        conns.push_back(c);
        std::cout << i << "号连接已建立" << std::endl;

        epoll_event ev;
        ev.events = EPOLLOUT | EPOLLIN | EPOLLET;
        ev.data.u32 = i; // 将当前的循环变量 i（连接序号）存入 epoll_event 的 data.u32 字段中
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
}

static void event_loop()
{
    std::vector<epoll_event> evs(CONN_N);
    while (true)
    {
        int nf = epoll_wait(epfd, evs.data(), evs.size(), 1);
        for (int i = 0; i < nf; ++i)
        {
            int idx = evs[i].data.u32; // 取出连接序号
            Conn *c = conns[idx];      // 找到对应连接
            if (evs[i].events & EPOLLIN)
                handle_recv(c);
            if (evs[i].events & EPOLLOUT)
                try_fill_pipe(c);
        }
        // 定时打印
        auto now = std::chrono::steady_clock::now();
        double sec = std::chrono::duration<double>(now - start_t).count();
        if (sec >= 1.0)
        {
            std::cout << "QPS=" << std::fixed << std::setprecision(0) << recv_cnt / sec
                      << "\t in-flight=" << g_inflight.load() << std::endl;
            send_cnt = recv_cnt = 0;
            start_t = now;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc >= 2)
        serv_ip = argv[1];
    if (argc >= 3)
        serv_port = atoi(argv[2]);
    start_t = std::chrono::steady_clock::now();
    create_connections();
    event_loop();
    return 0;
}