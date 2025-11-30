#pragma once

#include <vector>
#include <string>
#include <cassert>


//
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
//

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;   // 8字节前缀区域
    static const size_t kInitialSize = 1024; // 缓冲区初始大小为1024字节

    explicit Buffer(size_t initialSize = kInitialSize);
    ~Buffer() = default;

    size_t readableBytes() const;             // 获取当前可读的数据量
    size_t writableBytes() const;             // 获取当前写的数据量
    size_t prependableBytes() const;          // 获取m_readerIndex下标之前的空间大小
    const char* peek() const;                 // 获取当前可读数据的首地址
    int32_t peekInt32() const;                // 预读前4个字节的数据
    void retrieve(size_t len);                // 消费len长度的可读数据
    void retrieveAll();                       //消费所有可读数据
    std::string retrieveAsString(size_t len); // 消费长度为len的可读数据，并转换为string返回。
    std::string retrieveAllAsString();        //消费所有的可读数据，并转换为string返回
    void append(const char* data, size_t len);// 将长度为len的data尾插到m_buffer的写下标之后
    void append(const void* data, size_t len);// 将长度为len的data尾插到m_buffer的写下标之后
    void ensureWriteableBytes(size_t len);    // 确保缓冲区里能写下len长度的数据
    char* beginWrite();                       // 获取写下标的位置
    const char* beginWrite() const;           // 获取写下标的位置
    size_t readFd(int fd, int* savedError);   // 从内核缓冲区读取数据


private:
    char* begin();// 获取缓冲区起始地址
    const char* begin() const;// 获取缓冲区起始地址
    void makeSpace(size_t len); // 使缓冲区可以放得下len字节的数据。给缓冲区扩容或者移动m_readerIndex位置


    std::vector<char> m_buffer;
    size_t m_readerIndex; // 读下标，指向首个未读数据
    size_t m_writerIndex; // 写下标，指向首个待写位置
};