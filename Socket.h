#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socketfd
class Socket : noncopyable
{
public:
    // 防止隐式转换产生临时对象
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite(); // 关闭写端，优雅关闭fd

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};