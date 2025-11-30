#include "Buffer.h"

#include <sys/uio.h>
#include <errno.h>
#include <cstring>
#include <arpa/inet.h> 
Buffer::Buffer(size_t initialSize)
    : m_buffer(initialSize + kCheapPrepend),
      m_readerIndex(kCheapPrepend),
      m_writerIndex(kCheapPrepend)
{
    assert(initialSize > 0);
}


// 获取当前可读的数据量
size_t Buffer::readableBytes() const
{
    return m_writerIndex - m_readerIndex;
}

// 获取当前能写的数据量
size_t Buffer::writableBytes() const
{
    return m_buffer.size() - m_writerIndex;
}

// 获取m_readerIndex下标之前的空间大小
size_t Buffer::prependableBytes() const
{
    return m_readerIndex;
}

// 获取缓冲区起始地址
char* Buffer::begin()
{
    return &*m_buffer.begin();
}

// 获取缓冲区起始地址
const char* Buffer::begin() const
{
    return &*m_buffer.begin();
}

// 获取当前可读数据的首地址
const char* Buffer::peek() const
{
    return begin() + m_readerIndex;
} 

// 预读前4个字节的数据
int32_t Buffer::peekInt32() const
{
    int32_t result;
    memcpy(&result, peek(), sizeof(int32_t));
    return ntohl(result);
}


// 消费len长度的可读数据
void Buffer::retrieve(size_t len)
{
    if (len < readableBytes())
    {
        m_readerIndex += len;
    }
    else
    {
        retrieveAll();
    }
} 

//消费所有可读数据
void Buffer::retrieveAll()
{
    m_readerIndex = kCheapPrepend;
    m_writerIndex = kCheapPrepend;
}

// 消费长度为len的可读数据，并转换为string返回
std::string Buffer::retrieveAsString(size_t len)
{
    if (len <= readableBytes())
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    else
    {
        std::string result(peek(), readableBytes());
        retrieve(readableBytes());
        return result;
    }
}

//消费所有的可读数据，并转换为string返回
std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes());
} 
// 将长度为len的data尾插到m_buffer的写下标之后
void Buffer::append(const char* data, size_t len)
{
    ensureWriteableBytes(len);
    std::copy(data, data+len, beginWrite());
    m_writerIndex += len;
}

// 将长度为len的data尾插到m_buffer的写下标之后
void Buffer::append(const void* data, size_t len)
{
    append(static_cast<const char*>(data), len);
}

// 确保缓冲区里能写下len长度的数据
void Buffer::ensureWriteableBytes(size_t len)
{
    if (writableBytes() < len)
    {
        makeSpace(len);
    }
}

// 获取写下标的位置
char* Buffer::beginWrite()
{
    return begin() + m_writerIndex;
} 

// 获取写下标的位置
const char* Buffer::beginWrite() const
{
    return begin() + m_writerIndex;
}

// 使缓冲区可以放得下len字节的数据。给缓冲区扩容或者移动m_readerIndex位置
void Buffer::makeSpace(size_t len)
{
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) // m_buffer所有空闲空间都不足以放下长度为len字节的数据
    {
        m_buffer.resize(len + m_writerIndex); // vector底层容器扩容，时间消耗较大
    }
    else // 将 kCheapPrepend与m_readerIndex之间的区域 和 m_writerIndex与size() 之间的区域合并起来
    {
        size_t readable = readableBytes();
        std::copy(begin()+m_readerIndex, begin()+m_writerIndex, begin()+kCheapPrepend);
        m_readerIndex = kCheapPrepend;
        m_writerIndex = m_readerIndex + readable;
    }
} 

// 从内核缓冲区读取数据
size_t Buffer::readFd(int fd, int* savedError) 
{
    char extrabuf[65536] = {0}; //64KB，处于栈上
    struct iovec vec[2];
    size_t writable = writableBytes();

    // 存放内核读缓冲区数据的 区域1
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;

    // 存放内核读缓冲区数据的 区域2
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 如果缓冲区当前可写的空间发现小于64KB，则启动区域2来辅助接收数据
    const int iovcnt = writable < sizeof(extrabuf) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);
    if (n == -1)
    {
        *savedError = errno;
    }
    else if (n < writable)
    {
        m_writerIndex += n;
    }
    else
    {
        m_writerIndex = m_buffer.size(); 
        append(extrabuf, n-writable); // 将区域2中的数据添加到buffer缓冲区
    }

    return n;
}
