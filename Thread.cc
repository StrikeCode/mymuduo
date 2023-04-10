#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

// 原子变量初始化最好要这么写，不要写成 = 0,会报错，因为拷贝赋值被deleted
std::atomic_int Thread::numCreated_(0); 
// 定义中不用再次出现默认值
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach(); // 设置分离线程，作为后台线程了
    }
}

// 一个Thread对象，记录的就是一个新线程的详细信息
void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程的tid值
        tid_ = CurrentThread::tid();
        sem_post(&sem); // 信号量资源 + 1
        // 执行新线程的工作函数
        func_();
    }));
    
    // 这里必须等待获取上面新建的tid值
    // 防止上面tid_还是空的
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}