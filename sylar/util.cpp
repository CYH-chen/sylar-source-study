/**
 * @file util.cpp
 * @brief 工具函数实现
 * @version 0.1
 * @date 2026-01-13
 */
#include "util.h"

namespace sylar{
    
pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    // 假设已经有了，后面需要改为协程号
    return 0;
}
}