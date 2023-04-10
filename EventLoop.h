#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类： 负责 Channel Poller（epoll抽象）
class EventLoop : noncopyable
{
public: 
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop操作（epoll_wait）所在线程执行cb
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在线程
    void wakeup();

    // EventLoop的方法，转调用Poller对应方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    void hasChannel(Channel *channel);

    // 判断当前线程是否为loop操作所在的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void handleRead(); // 唤醒
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作 CAS实现
    std::atomic_bool quit_; // 标识是否退出loop循环

    // 调用某个loop对象的线程未必是进行loop操作的线程
    const pid_t threadId_; // 记录当前EventLoop进行loop操作所在线程的tid，确保Channel回调在其对应的evnetloop中执行 

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // 主要作用：当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该fd唤醒对应subloop来执行Channel回调
    std::unique_ptr<Channel> wakeupChannel_; // 别的线程唤醒本loop线程使用的Channel

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否正在执行回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调函数
    std::mutex mutex_; // 保护 pendingFunctors_ 线程安全操作

};