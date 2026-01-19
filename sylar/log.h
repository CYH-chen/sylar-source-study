/**
 * @file log.h
 * @brief 日志系统
 * @version 0.1
 * @date 2026-01-13
 */
#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
// 可变参数
#include <stdarg.h>
// C++20中获取行号 / 文件 / 函数信息的库
#include <source_location>
#include "util.h"
#include "singleton.h"

/**
 * @brief 使用宏定义来简化写入日志内容的过程
 * 
 * sylar::LogEvent::ptr()的用法会导致临时shared_ptr
 * 临时shared_ptr的生命周期是到这一整条表达式结束为止
 * 因此会导致未定义行为,需要使用Wrap进行包装
 * 
 */
#define SYLAR_LOG_LEVEL(logger , level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(logger, sylar::LogEvent::ptr(new sylar::LogEvent( \
            logger->getName(), level, std::source_location::current().file_name(), std::source_location::current().line(), \
            0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)                                             

// __VA_ARGS__ 是预定义的宏占位符，表示：调用宏时传给 ... 的那一整组参数
// 通过event中的format方法，使用 类似printf的格式 将日志写入logger
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(logger, sylar::LogEvent::ptr(new sylar::LogEvent(logger->getName(), level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

// 宏定义简化获取LoggerManager中默认日志器的接口
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
// 根据名称name获取日志器
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)
// 删除日志器，删除之前先将其level赋值为UNKNOW,且清除appender
#define SYLAR_LOG_ERASELOG(loggerName) sylar::LoggerMgr::GetInstance()->eraseLogger(loggerName)
// 获取所有日志器信息转为yaml
#define SYLAR_LOG_TOYAMLSTRING() sylar::LoggerMgr::GetInstance()->toYamlString()
namespace sylar {

// 日志级别
class LogLevel{
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };
    /**
     * @brief  将日志级别转成文本输出
     * 
     * @param level 日志级别
     * @return const char* 
     */
    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};

// 日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::string loggerName, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse,
            uint32_t threadId, uint32_t fiberId, uint32_t time);
    // ~LogEvent();

    const char* getFile() const { return m_file;}
    int32_t getLine() const { return m_line;}
    uint32_t getElapse() const { return m_elapse;}
    uint32_t getThreadId() const { return m_threadId;}
    uint32_t getFiberId() const { return m_fiberId;}
    uint64_t getTime() const { return m_time;}
    // 输出日志
    std::string getContent() const { return m_ss.str();}
    std::stringstream& getSS() { return m_ss;}
    const std::string& getLoggerName() const { return m_loggerName;}
    LogLevel::Level getLevel() const { return m_level;}

    // 可用fmt库或者C++20的format库进行替换
    // 格式化写入日志内容
    void format(const char* fmt, ...);
    // 格式化写入日志内容
    void format(const char* fmt, va_list al);
private:
    /// 日志器名称
    std::string m_loggerName;
    /// 日志等级
    LogLevel::Level m_level;
    // 文件名
    const char* m_file = nullptr;   
    // 行号
    int32_t m_line = 0;             
    // 程序启动开始到现在的毫秒数
    uint32_t m_elapse = 0;          
    // 线程id
    uint32_t m_threadId = 0;  
    // 协程id      
    uint32_t m_fiberId = 0;  
    // 时间戳       
    uint64_t m_time = 0;  
    // 内容          
    std::stringstream m_ss;
};

class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    // 根据pattern模式字符串进行格式化
    // 有默认值
    LogFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");   

    std::string format(LogEvent::ptr event);
public:
    // 子模块
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        FormatItem(const std::string& fmt = "" ) {}
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, LogEvent::ptr event) = 0;
    };
    /**
     * @brief 是否有错误
     * 
     * @return true 有错误
     * @return false 无错误
     */
    bool isError() const { return m_error;}
    const std::string& getPattern() const { return m_pattern;}
    /**
     * @brief pattern的解析
     * 
     */
    void init();
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    /// 是否有错误
    bool m_error = false;
};

// 日志输出地
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    LogAppender(LogFormatter::ptr formatter);
    virtual ~LogAppender() {}

    // 纯虚函数
    virtual void log(LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;
     
    // 获取日志级别
    LogLevel::Level getLevel() const { return m_level;}
    // 设置日志级别
    void setLevel(LogLevel::Level val) { m_level = val;}
    void setFormatter(LogFormatter::ptr val) { m_formatter = val;}
    LogFormatter::ptr getFormatter() { return m_formatter; }
protected:
    // 默认为DEBUG
    // 通过level可以控制Appender不同的输出级别
    // 举例：比如fileAppender只输出高级别日志，而stdOutAppender输出全级别 
    LogLevel::Level m_level = LogLevel::DEBUG;
    // 日志格式器
    LogFormatter::ptr m_formatter;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    StdoutLogAppender(LogFormatter::ptr formatter = std::make_shared<LogFormatter>());
    void log(LogEvent::ptr event) override;
    std::string toYamlString() override;

private:
};

//输出到文件的Appender
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename, LogFormatter::ptr formatter = std::make_shared<LogFormatter>());
    void log(LogEvent::ptr event) override;
    std::string toYamlString() override;

    // 重新打开文件，文件成功打开返回ture，反之false
    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
};

// 日志器
class Logger{
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name = "root", LogLevel::Level level = LogLevel::DEBUG);
    // ~Logger();
    // 使用日志器本身level的log
    void log(LogEvent::ptr event);
    // 自定level阈值的log（另一套方法）
    void log(LogLevel::Level level, LogEvent::ptr event);
    std::string toYamlString();

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }
    std::string getName() const { return m_name; }

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
private:
    //日志名称
    std::string m_name;  
    //日志级别                       
    LogLevel::Level m_level;     
    //Appender集合               
    std::list<LogAppender::ptr> m_appenders; 
};

class LogEventWrap {
public:
    LogEventWrap(Logger::ptr logger, LogEvent::ptr event);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event;}
    std::stringstream& getSS();
private:
    // 日志器
    Logger::ptr m_logger;
    // 日志事件
    LogEvent::ptr m_event;
};

// 负责管理Logger，需要Logger直接从里面取
class LoggerManager{
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& loggerName);
    /**
     * @brief 软删除，将其level赋值为UNKNOW,且清除appender
     * 防止失去管理
     * @param loggerName Logger名
     */
    void eraseLogger(const std::string& loggerName);
    std::string toYamlString();

    Logger::ptr getRoot() const { return m_root;}
private:
    // logger集合，每个string对应一个logger 
    std::map<std::string, Logger::ptr> m_loggers;
    // 默认logger
    Logger::ptr m_root;
};

// 包装类型名称LoggerMgr
typedef sylar::Singleton<LoggerManager> LoggerMgr;

// 在config.cpp中通过全局静态变量进行初始化
}

#endif