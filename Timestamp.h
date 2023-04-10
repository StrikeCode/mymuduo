#pragma once

#include <iostream>
#include <string>

class Timestamp
{
public:
    Timestamp();
    // explicit 防止类构造函数的隐式自动转换.参数个数 > 1 就失效了
    explicit Timestamp(int64_t microSecondsSinceEpoch) ; 
    static Timestamp now();
    // 将时间转换为年月日时分秒的格式
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};