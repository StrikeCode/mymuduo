#pragma once

#include <vector>
#include <string>
#include <algorithm>

class Buffer
{
public:
    static const size_t kCheapPrepend = 8; //
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writeableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区可读数据起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            // 应用只读取了可读缓冲区数据的部分，即len长度
            // 还剩[readerIndex_ + len, writerIndex_]区间长度数据没读
            readerIndex_ += len;
        }
        else 
        {
            retriveAll();
        }
    }

    void retriveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage上报的Buffer数据，转成string类型返回
    // 取出所有 readable 的数据转换为string返回  
    std::string retriveAllAsString()
    {
        return retriveAsString(readableBytes());
    }

    std::string retriveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 读完缓冲区各个指针要置位
        return result;
    }

    // 确保有长度为len的写缓冲区可用
    void ensureWriteableBytes(size_t len)
    {
        if(writeableBytes() < len)
        {
            makeSpace(len); // 扩容
        }
    }

    // 把[data, data+len]内存上的数据，添加到readable缓冲区中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len); 
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len; // 移动缓冲区可写的起始位置
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读数据
    ssize_t readFd(int fd, int *saveErrno);

    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char* begin()
    {
        return &*buffer_.begin(); // vector底层数组首元素的地址
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }

    // 扩充写缓冲区空间
    void makeSpace(size_t len)
    {
        // 根据buffer结构图理解即可
        // writeableBytes() + prependableBytes() - kCheapPrepend < len 
        // 可写空间不够
        if(writeableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else // 腾挪可读数据到前面，因为前面部分读完了可以复用空间
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_; // 
    // 注意：读写缓冲区都有可读和可写起始位置！！！
    size_t readerIndex_; // 可读数据起始位置
    size_t writerIndex_; // 可写入起始位置
};