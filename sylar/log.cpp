/**
 * @file log.cpp
 * @brief 日志系统实现
 * @version 0.1
 * @date 2026-01-13
 */
#include "log.h"
#include <functional>
#include <time.h>

namespace sylar{

/**
 *************************** LogEvent类实现 **************************
 * 
 */

LogEvent::LogEvent(std::string loggerName, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse,
            uint32_t threadId, uint32_t fiberId, uint32_t time            )
            :m_loggerName(loggerName)
            ,m_level(level)
            ,m_file(file)
            ,m_line(line)
            ,m_elapse(elapse)
            ,m_threadId(threadId)
            ,m_fiberId(fiberId)
            ,m_time(time) {

}

void LogEvent::format(const char* fmt, ...) {
    // 可变参数列表
    va_list al;
    // 初始化 al,可变参数在fmt后
    va_start(al, fmt);
    format(fmt, al);
    // 清理 va_list，必须调用（成对规则）
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}


/**
 *************************** LogLevel类实现 **************************
 * 
 */

const char* LogLevel::ToString(LogLevel::Level level) {
    switch (level)
    {
    // 使用宏定义简化switch-case
    #define XX(name) \
        case LogLevel::name: \
            return #name; \
            break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
    #undef XX
        default:
            return "UNKNON";
    }
}

/**
 *************************** LogFormatter类以及FormatItem派生类实现 **************************
 * 
 */

class MessageFormatItem : public LogFormatter::FormatItem{
public:
    // MessageFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem{
public:
    // LevelFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << LogLevel::ToString(event->getLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem{
public:
    // ElapseFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem{
public:
    // NameFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getLoggerName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
    // ThreadIdFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem{
public:
    // NewLineFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    // FiberIdFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        // 如果当%d没加{}时，默认值会被init()置空，所以需要进行二次判空赋值
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        // 把 time_t 转换为“本地时间”的 tm 结构
        localtime_r(&time, &tm);
        char buf[64];
        // 按照指定格式，把 tm 转成字符串
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    // FilenameFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    // LineFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    // TabFormatItem(const std::string& str = "") {}
    // 使用基类的构造函数，满足map中的接口要求，实际无作用
    using LogFormatter::FormatItem::FormatItem;
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << "\t";
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};


LogFormatter::LogFormatter(const std::string& pattern) 
    :m_pattern(pattern) {
    // init()对m_pattern进行解析，并调用相关formatterItem类进行输出
    init();

}

std::string LogFormatter::format(LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, event);
    }
    return ss.str();
}

// 只有%xxx %xxx{fmt} %% 这三种情况是转义字符
void LogFormatter::init() {
    //str, format, type
    // str ：文本内容或占位符名字
    // format ：{} 中的格式（可为空）
    // type ：0 → 普通文本; 1 → 占位符
    std::vector<std::tuple<std::string, std::string, int> > vec;
    // nstr 普通文本
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); i++) {
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        // 如果i+1没有越界，且为%（即 %% 的情况）
        if((i+1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                // 前移一位，跳过%%
                i++;
                continue;
            }
        }

        // 现在是 i 为 % ，且 i + 1 不为 %
        // 解析占位符 %xxx{fmt}
        size_t n = i + 1;
        // 解析格式标志
        int fmt_status = 0;
        // 格式字符串起始位置-1
        size_t fmt_begin = 0;
        // 占位符str
        std::string str;
        // 格式字符串
        std::string fmt;
        // 解析 % 后面的内容
        while(n < m_pattern.size()) {
            // 遇到 非字母、非 {} → 占位符名结束
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                // 此时 n 在 占位符+1 的位置
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }

            // 进入 {} 格式解析
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    // n-(i+1)
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1; 
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }
            else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    // n-(fmt_bein+1)
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            // {}解析完毕 或 无{}
            // n 开始右移
            ++n;
            // 处理字符串结束情况，%后的n一旦+1就结束，代表只有一位占位符
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }
        
        // 解析 % 之后的内容完毕， 开始压入vec
        if(fmt_status == 0) {
            // 先压入普通文本
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear();
            }
            // 再压入转义字符
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        else if(fmt_status == 1) {
            std::cout << "!!! pattern parse error: " << m_pattern << " -> " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }
    // 若结尾剩余普通文本
    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    // 这里由于format是virtual虚函数，因此即使是FormatItem::ptr仍可识别出相应的重载函数
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& fmt)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); } }

        // 这里为了符合时间Item需要传入一个字符串初始化
        // 其余item必须也实现一个 “含一个字符串参数的构造函数”
        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
        // XX(N, ThreadNameFormatItem),        //N:线程名称
#undef XX
    }; 

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            // 0 代表普通字符串
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            // 1 是转义字符
            // 通过占位符找到对应的处理函数
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                // 格式错误
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                // 将对应item压入，并传入构造字符串
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
}

/**
 *************************** Logger类实现 **************************
 * 
 */

/**
 * name 默认值 root
 * level 默认值 LogLevel::DEBUG
 */
Logger::Logger(const std::string& name, LogLevel::Level level) 
    :m_name(name) 
    ,m_level(level) {
    // 设置输出格式 
    // formatter有默认值,可以不用reset了
    // m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for(auto it = m_appenders.begin();
            it!= m_appenders.end(); it++) {
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::log(LogEvent::ptr event) {
    // m_level才是logger的级别
    // level是event的级别,只要event的级别大就输出
    if(event->getLevel() >= m_level){
        for(auto& i : m_appenders) {
            i->log(event);
        }
    }
}

// 为适应下面另一套方法的log函数
void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    // m_level才是logger的级别
    // level是event的级别,只要event的级别大就输出
    if(level >= m_level){
        for(auto& i : m_appenders) {
            i->log(event);
        }
    }
}

// 另一套方法
void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

/**
 *************************** LogEventWrap类实现 **************************
 * 
 */

LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event)
    :m_logger(logger)
    ,m_event(event) {

}
LogEventWrap::~LogEventWrap() {
    m_logger->log(m_event);
}

std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}

/**
 *************************** LogAppender以及其派生类实现 **************************
 * 
 */

LogAppender::LogAppender(LogFormatter::ptr formatter)
    :m_formatter(formatter) {
}

StdoutLogAppender::StdoutLogAppender()
    :LogAppender(LogFormatter::ptr(new LogFormatter)) {

}

void StdoutLogAppender::log(LogEvent::ptr event) {
    if(event->getLevel() >= m_level) {
        std::cout << m_formatter->format(event);
    }

}

FileLogAppender::FileLogAppender(const std::string& filename) 
    :LogAppender(LogFormatter::ptr(new LogFormatter)) {
    m_filename = filename;

    reopen();
}

bool FileLogAppender::reopen() {
    if(m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

void FileLogAppender::log(LogEvent::ptr event) {
    if(event->getLevel() >= m_level) {
        m_filestream << m_formatter->format(event);
    }
}

/**
 *************************** LoggerManager类实现 **************************
 * 
 */

LoggerManager::LoggerManager() {
    // 初始化默认logger
    // logger构造函数有默认参数name和level
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
}

Logger::ptr LoggerManager::getLogger(const std::string& loggerName) {
    auto it = m_loggers.find(loggerName);
    return it == m_loggers.end() ?  m_root : it->second;
}

}