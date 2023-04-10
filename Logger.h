#pragma once
#include <string>

#include "noncopyable.h"

// ???此处可优化，将主体内容写入log函数
// 对外使用方法 LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) 

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    }while(0) 

// 只有定义了 MUDEBUG 才以DEBUG级别输出日志
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) 
#else 
    #define LOG_DEBUG(logmsgFormat, ...) 
#endif

// 定义日志级别,枚举是整型，默认第一项从0开始
enum LogLevel
{
    INFO,   // 普通信息
    ERROR,  // 错误信息
    FATAL,  // core的信息
    DEBUG,  // 调试信息，量最大
};

class Logger : noncopyable
{
public:
    // 获取日志唯一实例对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    int logLevel_; // 结尾_ 防止和系统定义的变量混淆
};