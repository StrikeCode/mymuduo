#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

// 头文件中只给类的前置声明，而在源文件中再给出头文件包含
// 因为源文件会被编程动态库.so, 减少对外暴露
class EventLoop;

// EventLoop包含多个Channel 和 Poller
// Channel对应Reactor上的 Demultiplex （多路复用器）
// !!!一个channel对应唯一EventLoop，一个EventLoop可以有多个channel
// Channel理解为通道
// Channel是muduo库负责注册读写事件的类，并保存了fd读写事件发生时调用的回调函数
// 如果poll/epoll有读写事件发生则将这些事件添加到对应的通道中。
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知后调用其处理事件
    void handleEvent(Timestamp recevieTime);

    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当Channel的所有者被手动remove掉时，Channel 仍在执行回调
    void tie(const std::shared_ptr<void>&); // 检测资源存活状态

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    // enableReading 让fd对读事件感兴趣
    // update()底层也是调用 epoll_ctl
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; }

    // 返回fd当前关注的事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting()const { return events_ & kWriteEvent; }
    bool isReading()const { return events_ & kReadEvent; }
    
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // oneloop per thread
    // 当前Channel所属的eventloop
    EventLoop* ownerLoop() {return loop_;}
    void remove();

private:

    void update();
    void handleEventWithGuard(Timestamp recvTime);

    static const int kNoneEvent; // 感兴趣的事件类型，该变量表示不感兴趣任何事件
    static const int kReadEvent; 
    static const int kWriteEvent; 

    EventLoop *loop_; // 事件循环
    const int fd_;  //fd， poller监听的对象
    int events_;    // 注册感兴趣的事件
    int revents_;   // poller返回的具体发生的事件类型（可读？可写？）
    int index_;

    std::weak_ptr<void> tie_; // 用于观察shared_ptr的状态
    bool tied_;

    // 因为Channel里能得知fd最终发生的具体事件revents_
    // 故它负责调用对应的回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};