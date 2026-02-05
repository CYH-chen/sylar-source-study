/**
 * @file fiber.h
 * @brief 协程模块
 * @version 0.1
 * @date 2026-02-04
 * 
 */
#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace sylar {
/**
 * @brief 非对称协程
 * Thread ---> main_fiber <---> sub_fiber
 *                 ^
 *                 |
 *                 |
 *                 v
 *             sub_fiber
 * 通过main_fiber创建协程，子协程完毕后返回main协程调度其他协程
 * 适合io密集型
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
private:
    /**
     * @brief 线程的主协程构造
     * 
     */
    Fiber();

public:
    /**
     * @brief 构造子协程。stacksize = 0 时，会使用配置文件中的stacksize
     * uc_link = nullptr，任何 Fiber 切换，都必须显式走调度器
     * @param cb 
     * @param stacksize 
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();
    /**
     * @brief 可用于重置协程，在INIT或TERM状态
     * 充分利用内存，当协程的内存空间未释放且空闲的时候，直接重置cb，开始一个新任务。
     * 
     * @param cb 执行函数
     */
    void reset(std::function<void()> cb);
    /**
     * @brief 切换成当前协程
     * 
     */
    void swapIn();
    /**
     * @brief 让出cpu，切换成主协程
     * 
     */
    void swapOut();

    uint64_t getId() { return m_id; }

public:
    /**
     * @brief Get the This object
     * 
     * @return Fiber::ptr 智能指针
     */
    static Fiber::ptr GetThis();
    /**
     * @brief Set the This object
     * 
     * @param fiber 传this裸指针进去，不用智能指针
     */
    static void SetThis(Fiber* fiber);
    /**
     * @brief 获取当前协程的id，若当前协程为nullptr返回0
     * 为什么不用GetThis()->getId()：因为有些线程可能协程，调用GetThis()会创建主协程
     * 
     * @return uint64_t 
     */
    static uint64_t GetFiberId();
    /**
     * @brief 让出cpu，转为READY态
     * 
     */
    static void YieldToReady();
    /**
     * @brief 让出cpu，转为HOLD态
     * 
     */
    static void YieldToHold();

    // 获取用于服务器统计分析的信息
    /**
     * @brief 获取总协程数
     * 
     * @return uint64_t 总协程数
     */
    static uint64_t TotalFibers();
    /**
     * @brief 协程要执行的函数
     * 
     */
    static void MainFunc();

private:
    uint64_t m_id= 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};
    
}

#endif