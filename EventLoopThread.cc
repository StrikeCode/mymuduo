#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
        const std::string &name)
        : loop_(nullptr)
        , exiting_(false)
        , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
        , mutex_()
        , cond_()
        , callback_(cb)
{

}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层的新线程
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 用while防止被其他线程抢走了，确保资源存在
        while(loop_ == nullptr) // 等待对应的 EventLoop创建
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 在单独的新线程里运行，即thread_.start()内的func()
void EventLoopThread::threadFunc()
{
    // 创建一个独立的eventloop，和上面线程一一对应
    // one loop per thread
    // 栈上分配
    EventLoop loop; 

    if(callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop.loop => Poller.poll
    // loop 内 quit被置为true，退出loop才会到下面代码
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}