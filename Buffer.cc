#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

// 从fd上读数据, Poller工作在LT模式
// Buffer缓冲区有大小！ 但从fd读数据时，不知道tcp数据最终大小
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上内存空间 64k

    struct iovec vec[2];
    const size_t writeable = writeableBytes(); // 这是Buffer底层缓冲区剩余可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // writable < sizeof extrabuf 表示底层可写的缓冲区空间不够大，用两块
    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt); // scatter input，分散读
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writeable) // Buffer的可写缓冲区够存储读出来的数据
    {
        writerIndex_ += n; // 读数据当然writerIndex_后移，结合图
    }
    else // extrabuf里面也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writeable);
    }
    return n;
}

// 通过fd发送数据
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    // 可读的数据通过fd发出去
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}