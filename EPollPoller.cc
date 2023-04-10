#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <unistd.h>
#include <errno.h>
#include <strings.h>

// channel未添加到poller的ChannelMap中
const int kNew = -1; // channel成员初始index_ = -1
// channel已添加到poller的事件监视中。即，上epoll树监视了。
const int kAdded = 1;
// channel 从poller的监视列表删除但仍在poller的 ChannelMap 中,但还在eventLoop的ChannelList中
const int kDeleted = 2;

// EPOLL_CLOEXEC使得exec时关闭无效的文件描述符
EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(KInitEventListSize)
{
    if(epollfd_ < 0)
    {
        // 记录后直接退出
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_INFO("func = %s => fd total count : %lu \n", __FUNCTION__, channels_.size());

    // 
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno; // 防止错误码执行中被修改
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        // 将活跃(有事件发生的)Channel添加到activeChannels中
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size()) // 扩容
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel
//          EventLoop
//  ChannelList     Poller
//                  ChannelMap<fd, channel*> epollfd
void EPollPoller::updateChannel(Channel *channel)
{
    // index() 得出channel状态，即kNew or kAdded or kDeleted
    const int index = channel->index();
    LOG_INFO("func = %s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted)
    {
        if(index == kNew) // 加入Poller的ChannelMap
        {
            int fd = channel->fd();
            channels_[fd] = channel; //
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel已经在poller上注册过，即加入了 ChannelMap
    {
        int fd = channel->fd();
        if(channel->isNoneEvent()) // 没有感兴趣的事件，从内核事件表删除
        {
            update(EPOLL_CTL_DEL, channel);
            // kDeleted表示Poller不监视该channel，但未从ChannelMap中删除
            channel->set_index(kDeleted);
        }
        else 
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
// ???从poller中拆除通道
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd); // 从ChannelMap中删除，成为kNew

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if(index == kAdded) 
    {
        update(EPOLL_CTL_DEL, channel);
    }
    // 注意这里设置的时kNew而不是kDeleted
    channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop获得了它的poller给他返回的所有发生事件的channel列表
    }
}

// 供updateChannel调用
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else // 视mod和add出错为致命的错误
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
