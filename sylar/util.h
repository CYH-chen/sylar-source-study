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

namespace sylar{

pid_t GetThreadId();
uint32_t GetFiberId();
}

#endif