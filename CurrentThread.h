#pragma once

// 该头文件封装获取当前线程的函数
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // __thread实现每个线程中都有一份独立的 t_cachedTid 实例，互不干扰
    extern __thread int t_cachedTid;
    
    void cacheTid();

    inline int tid()
    {   // 告诉编译t_cachedTid == 0 不大可能成立
        // 表达式还是相当于 t_cachedTid == 0
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}