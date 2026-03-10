/**
 * @file scheduler.h
 * @brief 协程调度器模块
 * @version 0.1
 * @date 2026-02-12
 * 
 */
#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <list>
#include <atomic>

#include "mutex.h"
#include "thread.h"
#include "fiber.h"

namespace sylar {

// 有栈非对称协程
// 协程调度器基类
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;
    /**
     * @brief 创建一个协程调度器。
     * @attention 仅进行参数配置，需要start()
     * 
     * @param threads 线程数
     * @param use_caller 是否将当前线程加入调度器
     * @param name 调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name;}

    /**
     * @brief 获得当前的协程调度器
     * 
     * @return Scheduler* 
     */
    static Scheduler* GetThis();
    /**
     * @brief 获得调度器的主协程，用于进行调度操作
     * @attention 与Fiber.cpp中的主协程不太一样，该主协程是执行run()的协程
     * 
     * @return Fiber* 
     */
    static Fiber* GetMainFiber();
    /**
     * @brief 线程池启动
     * 
     */
    void start();
    /**
     * @brief 停止所有活动
     * 
     */
    void stop();

    // 调度器执行任务的方法
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, pid_t thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        
        if(need_tickle) {
            // 唤醒
            tickle();
        }
    }
    // 调度器执行任务的方法的批量版本，只需锁一次即可放入所有任务，保证任务连续
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end)
            {   
                // 会转移原容器中任务的所有权
                need_tickle = scheduleNoLock(std::move(*begin)) || need_tickle;
                begin++;
            }
            
            if(need_tickle) {
            // 唤醒
            tickle();
            }
        }
    }

protected:
    /**
     * @brief 唤醒函数
     * 
     */
    virtual void tickle();
    /**
     * @brief 线程入口函数：进行任务调度
     * 
     */
    void run();
    /**
     * @brief 指示是否停止
     * 
     * @return true 
     * @return false 
     */
    virtual bool stopping();
    /**
     * @brief idler协程，空闲时执行。
     * 解决的是：协程调度器空闲时，不让线程终止。
     * 具体实现由子类完成
     * 
     */
    virtual void idle();

    void setThis();
private:
    /**
     * @brief 调度方法的无锁版本
     * 
     * @tparam FiberOrCb 协程fiber or callback
     * @param fc 协程fiber or callback
     * @param thread 线程id
     * @return true 唤醒线程
     * @return false 不进行唤醒操作
     */
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, pid_t thread) {
        // 若放入前，任务组为空，说明此时所有线程都是阻塞态，因为没有任务可运行
        bool need_tickle = m_tasks.empty();
        // FiberAndThread中对fc进行完美转发，即可值拷贝也可转移所有权
        Task ft(fc, thread);
        // 如果fiber或cb存在，就加入任务组
        if(ft.fiber || ft.cb) {
            m_tasks.push_back(ft);
        }
        // 是否需要进行唤醒操作
        return need_tickle;
    }
private:
    // 可执行的结构体
    struct Task {
        Fiber::ptr fiber;
        std::function<void()> cb;
        // 线程ID，可实现协程指定在某一个线程执行
        pid_t thread;

        // 用完美转发兼容值传递和转移所有权，相比使用“std::move”或“智能指针的指针”性能稍微好一点点
        template<typename Ptr>
        requires std::same_as<std::decay_t<Ptr>, Fiber::ptr>
        Task(Ptr&& f, pid_t thr)
            :fiber(std::forward<Ptr>(f)), thread(thr) {}
        // std::invocable<F> : 类型 F 是否可以像函数一样被调用
        template<typename F>
        requires std::invocable<F> &&
        (!std::same_as<std::decay_t<F>, Fiber::ptr>)
        Task(F&& f, pid_t thr)
            :cb(std::forward<F>(f)), thread(thr) {}

        // 默认构造函数，防止无法占位初始化
        Task() 
            :fiber(nullptr), thread(-1) {}

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
    
private:
    MutexType  m_mutex;
    // 线程池，装载的是协程中管理的线程
    std::vector<Thread::ptr> m_threads;
    // 协程的消息队列。计划执行的任务（协程fiber或回调函数cb）
    std::list<Task> m_tasks;
    // 调度协程 
    Fiber::ptr m_rootFiber;
    std::string m_name;

protected:
    std::vector<int> m_threadIds;
    // 只有初始化时才会写，不涉及多线程修改
    size_t m_threadCount = 0;
    // 多线程同时修改
    std::atomic<size_t> m_activeThreadCount{0};
    std::atomic<size_t> m_idleThreadCount{0};
    std::atomic<bool> m_stopping = true;
    // 是否自动停止
    std::atomic<bool> m_autoStop = false;
    // 主线程ID，use_caller的ID 
    int m_rootThreadId = 0;
};

}


#endif