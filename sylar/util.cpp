/**
 * @file util.cpp
 * @brief 工具函数实现
 * @version 0.1
 * @date 2026-01-13
 */
#include "util.h"
#include "log.h"
#include <execinfo.h>

namespace sylar{
    
sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    // 假设已经有了，后面需要改为协程号
    return 0;
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    // 不占用栈空间，协程的栈比较小。大对象用堆
    // void*是地址，32位系统是4字节，64位系统是8字节
    void** array = (void**)malloc((sizeof(void*) * size));
    // ::代表全局命名空间。返回的是真实存储了多少层
    size_t s = ::backtrace(array, size);

    // newly malloc()ed的空间，也需要free
    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
        free(array);
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
    return;
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

}