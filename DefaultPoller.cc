#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

// 依赖倒置
Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        // 生成POLL的实例
        return nullptr;
    }
    else 
    {
        // 生成EPOLL实例
        return new EPollPoller(loop);
    }
}