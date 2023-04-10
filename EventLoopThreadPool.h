#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    // ???谁来调用传入cb
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 若工作在多线程中，baseLoop_默认以轮询方式分配Channel给 subLoop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_; // 轮询用的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // pool
    std::vector<EventLoop*> loops_;
};