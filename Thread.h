#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>


class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    // Blocks the current thread until the thread identified by *this finishes its execution.
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    bool started_;
    // joined_ == true，表示必须等待线程执行工作函数后才能接下去的工作
    bool joined_; 
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_; // 记录产生线程的个数,给线程起名用的
};