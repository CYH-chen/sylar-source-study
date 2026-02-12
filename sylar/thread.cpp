/**
 * @file thread.cpp
 * @brief 线程模块实现
 * @version 0.1
 * @date 2026-01-21
 * 
 */
#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

// thread_local 变量具有线程存储期。每个线程都会拥有该变量的一个独立实例

// static 仅在当前 cpp 文件内可见（内部链接）
/**
 * @brief 当前线程对应的 Thread 对象指针。
 * 在线程内部，能随时知道当前跑在哪个 Thread 对象。
 */
static thread_local Thread* t_thread = nullptr;
// 当前线程的名字
static thread_local std::string t_thread_name = "UNKNOW";
// 通过单例模式获取全局唯一的数据
static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

// pthread（只认识 C 函数）
//         ↓
// static 成员函数（无 this）
//         ↓
// this 指针传递，将this指针作为run的参数
//         ↓
// 真实的对象成员函数
Thread::Thread(std::function<void()> cb, const std::string& name)     
    :m_cb(std::move(cb))
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    /**
     * pthread_t* thread,          // 输出参数：线程ID
     * const pthread_attr_t* attr,  // 线程属性（通常为 nullptr）
     * void* (*start_routine)(void*), // 线程入口函数
     * void* arg                    // 传给入口函数的参数
     */
    int ret = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(ret) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << ret
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    // 阻塞当前构造函数，确保创建的线程开始运行后，在退出构造函数。在run函数中会调用m_semaphore.notify()
    // 保证构造线程类完成之前，线程一定开始运行了
    m_semaphore.wait();
}

Thread::~Thread() {
    // 如果线程存在，就
    if(m_thread) {
            // 线程还在跑，并没有结束，线程退出时自动回收资源
            // 设置为脱离线程，因为Thread 析构 ≠ 线程生命周期结束
            pthread_detach(m_thread);
            // 不推荐join，join会阻塞。析构是用来兜底的，join可以作为另一个方法
        }
}

void Thread::join() {
    if(m_thread) {
        // 这是当前线程对Thread类管理的另一个线程使用join，join会阻塞
        int ret = pthread_join(m_thread, nullptr);
        if(ret) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << ret
            << " name=" << m_name;
        throw std::logic_error("pthread_join error");
        }
        // 要改为0，不然Segmentation fault (core dumped)
        // 因为此时join完毕，线程已经释放了，后面调用析构函数时，不能再次detach此线程
        m_thread = 0;
        // 此时非joinable状态
    }
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    // 当前线程即为进入run的线程
    t_thread = thread;
    // 对全局线程变量命名
    t_thread_name = thread->m_name;
    // 设置包装的线程类中的线程Id
    t_thread->m_id = GetThreadId();
    // 对线程进行命名。pthread_setname_np最多接受16个字符，包括\0
    pthread_setname_np(pthread_self(), t_thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    // 不再依赖 Thread 对象本身是否还活着，可以减少一次智能指针的引用计数
    cb.swap(t_thread->m_cb);
    // this只是裸指针，对引用计数无影响
    // run为static函数，因此即便Thread析构了，cb也仍然可以继续执行

    // 构造函数可以退出
    t_thread->m_semaphore.notify();

    cb();
    return 0;
}



}