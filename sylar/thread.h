/**
 * @file thread.h
 * @brief 线程模块
 * @version 0.1
 * @date 2026-01-20
 * 
 */
#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

namespace sylar {

// 信号量
class Semaphore {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();

private:
    // 只能引用传递
    Semaphore (const Semaphore&) = delete;
    Semaphore (Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    // 确保构造函数返回之前，线程已经开始运行 
    sem_t m_semaphore;
};

template<class T>
class ScopeLockImpl {
public:
    ScopeLockImpl(T& mutex) 
        :m_mutex(mutex) {
        lock();
    }

    ~ScopeLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

// 对写锁进行锁和解锁的RAII对象
template<class T>
class ReadScopeLockImpl {
public:
    ReadScopeLockImpl(T& mutex) 
        :m_mutex(mutex) {
        lock();
    }

    ~ReadScopeLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

// 对读锁进行锁和解锁的RAII对象
template<class T>
class WriteScopeLockImpl {
public:
    WriteScopeLockImpl(T& mutex) 
        :m_mutex(mutex) {
        lock();
    }

    ~WriteScopeLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

class Mutex {
public:

private:

};

// 管理读写锁生命周期的RAII对象
class RWMutex {
public:
    // 通过syalr::RWMutex::xxx调用，统一接口 
    typedef ReadScopeLockImpl<RWMutex> ReadLock;
    typedef WriteScopeLockImpl<RWMutex> WriteLock;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;

};

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