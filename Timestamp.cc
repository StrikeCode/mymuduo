#include "Timestamp.h"
#include <time.h>



Timestamp::Timestamp():microSecondsSinceEpoch_(0) {};

    // explicit 防止类构造函数的隐式自动转换.参数个数 > 1 就失效了
Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {}
Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}
// 将时间转换为年月日时分秒的格式
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);   
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);

    return buf;
}

// #include <iostream>
// int main()
// {
//     std::cout << Timestamp::now().toString() << std::endl;
//     return 0;
// }