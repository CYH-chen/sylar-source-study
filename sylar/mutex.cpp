/**
 * @file mutex.cpp
 * @brief 信号量和互斥锁的实现
 * @version 0.1
 * @date 2026-02-12
 * 
 */
#include "mutex.h"

namespace sylar {
    Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait(){
    // wait失败抛异常
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify(){
    // post失败抛异常
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}
} 

