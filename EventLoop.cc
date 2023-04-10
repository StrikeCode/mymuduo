#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 每个线程都有自己独立的一份拷贝 thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakefd，用来notify唤醒subReactor 处理新来的Channel
int createEventfd()
{
    // EFD_CLOEXEC处理fork()导致的fd泄漏
    // eventfd 用来创建用于事件 wait / signal 的fd
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid()) // 当前loop的线程是构造时的线程
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else 
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每个eventloop都将监听wakeupchannel_的EPOLLIN读事件
    // 等待被唤醒
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); // 对所有事件不感兴趣
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd，一种时clientfd，一种是wakeupfd(main reactor 和 sub reactor通信用)
        // 发生事件的Channel都被加入到 activeChannels_  中
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for(Channel *channel : activeChannels_)
        {
            // Poller监听哪些Channel发生事件，上报给EventLoop，通知Channel处理相应事件
            channel->handleEvent(pollReturnTime_);
        }

        // 执行当前EventLoop 事件循环需要处理的回调操作
        // IO线程 mainLoop accept fd 然后将连接fd对应的channel传递到 subloop
        // mainLoop事先注册回调cb（需要subloop所在线程中执行）
        // 通过 wakeupChannel_唤醒 subloop后 执行mainLoop事先注册回调cb
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环: 1) loop在自己的线程中调用quit； 2) 在非loop的线程中调用loop(构造loop的线程)的quit
void EventLoop::quit()
{
    quit_ = true;
    // 若在其他线程中调用quit(),则要唤醒当前loop线程进行退出
    // 场景如：在一个subloop中(worker) 中，调用了mainLoop(IO)的quit
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()) // 在当前的loop线程中，则执行cb
    {
        cb();
    }
    else // 在非当前loop线程中执行cb，需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}
// 把cb放入队列中，唤醒loop操作（epoll_wait）所在线程执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); // emplace_back减少拷贝开销
    }

    // 唤醒相应的，需要执行上面回调操作的loop线程
    // || callingPendingFunctors_ 表示： 当前loop正在执行回调，但是loop有了新的回调
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); // 唤醒使得下一次epoll_wait检测到Channel事件到来
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof(one))
    {
       LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n); 
    }
}

// 用来唤醒loop所在线程,也就是向wakeupfd_写8 bytes 数据
// wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8", n);  
    }
}

// EventLoop的方法，转调用Poller对应方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

void EventLoop::hasChannel(Channel *channel)
{
    poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 使得EventLoop::queueInLoop中往 pendingFunctors_ 加入回调不用等待这一批回调执行完就可加入
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调
    }
    callingPendingFunctors_ = false;
}