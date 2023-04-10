#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;
/*
    该类核心就是封装了epoll的操作：
    epoll_create
    epoll_ctl  add/mod/del
    epoll_wait
*/

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int KInitEventListSize = 16; // 事件链表初始大小
    // 填写活跃的连接到EventLoop中的ChannelList
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel对应socket的感兴趣的事件
    void update(int operation, Channel *channel);

    // 用vector方便扩容
    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};


