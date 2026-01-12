/**
 * @file test.cpp
 * @brief 日志系统测试
 * @version 0.1
 * @date 2026-01-13
 */
#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"

int main(int argc, char* argv[]) {
    sylar::Logger::ptr log(new sylar::Logger);
    // addAppender方法会检查是否有formatter
    log->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    // ERROR等级输出
    file_appender->setLevel(sylar::LogLevel::ERROR);
    log->addAppender(file_appender);

    sylar::LogEvent::ptr event(new sylar::LogEvent(log->getName(), sylar::LogLevel::DEBUG, __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), 
                                                time(0)));
    event->getSS() << "This is my log";
    
    log->log(event);
    std::cout << "hello world" << std::endl;

    SYLAR_LOG_INFO(log) << "测试宏定义";
    SYLAR_LOG_ERROR(log) << "ERROR宏定义测试";

    SYLAR_LOG_FMT_ERROR(log, "fmt宏定义： %s", "fmt测试");

    // 这里会返回一个默认logger，即管理器中的默认logger: m_root
    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_WARN(l) << "LoggerMrg 测试";
    return 0;
}