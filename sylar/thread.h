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

// 对互斥锁进行加锁和解锁的RAII对象
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

// 对写锁进行加锁和解锁的RAII对象
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

// 对读锁进行加锁和解锁的RAII对象
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

// 互斥锁
class Mutex {
public:
    typedef ScopeLockImpl<Mutex> Lock;

    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
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

/**
 * @brief 由于互斥锁mutex在日志模块中需要频繁地切换内核态进行线程调度
 * 导致运行速度比不使用锁降低了很多，因此考虑使用自旋锁(spinlock)。
 * 因为日志模块的冲突等待时间少（日志输出快），同时冲突次数多。
 * 适合使用自旋锁避免频繁进行用户态和内核态的切换
 * 
 * 使用spinlock提高写文件性能，从8M/s到20M/s
 * 
 * 也可以尝试使用C++的atomic库的自旋锁
 */
class Spinlock {
public:
    typedef ScopeLockImpl<Spinlock> Lock;

    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
 
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