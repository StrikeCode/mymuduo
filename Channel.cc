#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0; 
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; 
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{    
}

// ??channel的tie方法什么时候调用过
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// 当改变Channel所表示fd的events事件后，update负责在poller里更改fd相应事件--epoll_ctl
// EventLoop has ChannelList Poller
void Channel::update()
{
    // 通过Channel所属的EventLoop，调用Poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在Channel所属的EventLoop中，把当前Channel删除
void Channel::remove()
{
    loop_->removeChannel(this);
}

// fd得到poller通知后处理事件
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_) // 资源存活
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else 
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据Poller通知的Channel发生的具体事件，调用相应的回调
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    // EPOLLHUP 表示读写都关闭
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }

}
