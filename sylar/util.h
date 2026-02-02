/**
 * @file util.h
 * @brief 工具函数
 * @version 0.1
 * @date 2026-01-13
 */
#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace sylar{

pid_t GetThreadId();
uint32_t GetFiberId();

/**
 * @brief 将调用栈转为string，每一层存在vector中
 * 
 * @param bt 目标vector
 * @param size 栈大小（层数）
 * @param skip 跳过前面多少层，默认为1，跳过前面一层Backtrace调用
 */
void Backtrace(std::vector<std::string>& bt, int size, int skip = 1);

/**
 * @brief 直接将其转化为string
 * 
 * @param size 栈大小
 * @param skip 跳过前面多少层，默认为2，跳过前面两层Backtrace调用
 * @return std::string 
 */
std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "");
// const std::string&：若绑定右值（如 “hello”），C++ 会延长临时对象的生命周期至引用作用域结束

}

#endif