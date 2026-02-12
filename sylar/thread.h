/**
 * @file thread.h
 * @brief 线程模块
 * @version 0.1
 * @date 2026-01-20
 * 
 */
#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <pthread.h>

#include "mutex.h"

namespace sylar {

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    void join();

    const std::string& getName() const { return m_name;}
    const pid_t getId() const { return m_id;}

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    // 禁用拷贝构造和移动构造
    Thread (const Thread&) = delete;
    Thread (Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void* arg);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    // 被包装的可调用对象‌不接受任何参数‌且‌返回类型为void‌
    std::function<void()> m_cb;
    std::string m_name; 

    Semaphore m_semaphore;
};

}

#endif