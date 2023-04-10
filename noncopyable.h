#pragma once // 编译器级别防止头文件重复包含

/*
    noncopyable被继承后，派生类对象可正常构造
    但不能进行拷贝构造和赋值
    提升复用性
*/

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};