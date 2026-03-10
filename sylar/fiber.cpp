/**
 * @file fiber.cpp
 * @brief 协程模块实现
 * @version 0.1
 * @date 2026-02-04
 * 
 */
#include <atomic>
#include "fiber.h"
#include "log.h"
#include "config.h"
#include "macor.h"
#include "scheduler.h"

namespace sylar {

static std::atomic<uint64_t> s_fiber_id { 0 };
static std::atomic<uint64_t> s_fiber_count { 0 };

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 线程的当前协程
static thread_local Fiber* t_scheduler_fiber = nullptr;
// 线程的主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024*1024, "fiber stack size");

// 统一接口
class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    // 为了以后便于添加新的allocator，故保留size参数
    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

// 统一类型名，便于维护
using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    // 每个线程都有一个mainFiber，且id == 0
    ++s_fiber_count;
    m_state = EXEC;
    SetThis(this);

    // 获取上下文
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber mainFiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool isBackToCaller) 
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    // un_link是当前上下文结束后返回的上下文，为nullptr不绑定任何上下文
    // nullptr确保是显示调度，任何 Fiber 切换，都必须显式走调度器
    m_ctx.uc_link = nullptr;
    // 绑定分配的栈空间
    /**
     * 栈空间会保存其执行函数中的局部变量
     * 因此若其中有用到智能指针，需要手动释放
     * 不然计数永远+1，无法自动析构，同时反过来导致调用栈无法被释放，造成内存泄露（类似死锁）
     * 
     */
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    if(isBackToCaller) {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    } else {
        // 指定入口函数，真正切换用swapcontext
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    // 主协程没有栈空间 m_stack == nullptr
    if(m_stack) {
        // 有栈的情况下，协程状态要么状态是刚初始化，要么是结束了
        SYLAR_ASSERT(m_state == INIT || m_state == EXCEPT || m_state == TERM);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else {
        // 此时为主协程，没有callback函数且状态为EXEC
        SYLAR_ASSERT(!m_cb && m_state == EXEC);
        // 比较是否为当前协程，是的话将当前协程置为nullptr
        Fiber* cur =  t_scheduler_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
}

void Fiber::reset(std::function<void()> cb) {
    // 有栈且状态对的协程才能重置
    SYLAR_ASSERT(m_stack);
    // 协程只有在EXCEPT和TERM、INIT状态下才允许重置
    SYLAR_ASSERT(m_state == EXCEPT || m_state == TERM || m_state == INIT);
    // 转移cb
    m_cb = std::move(cb);

    // 下面与构造函数基本相同
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    m_state = INIT;
}

void Fiber::swapIn() {
    SetThis(this);
    // 必定不在运行态
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    
    // SYLAR_LOG_INFO(g_logger) << "从调度协程切入协程";
    // swapcontext(&old_context, &new_context)将当前状态保存到old_context中，切换到new_context
    if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext error");
    }
    
}

void Fiber::swapOut() {
    // SYLAR_LOG_INFO(g_logger) << "切回调度协程";
    // 切回调度协程
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
        SYLAR_ASSERT2(false, "swapcontext error");
    }
}

void Fiber::call() {
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext error");
    }
}


void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext error");
    }
}

/**
 * @brief 返回当前线程正在执行的协程。
 * 如果当前线程还未创建协程，则创建线程的第一个协程，且该协程为当前线程的主协程，其他协程都通过这个协程来调度；
 * 也就是说，其他协程结束时,都要切回到主协程，由主协程重新选择新的协程进行resume。
 * @attention 线程如果要创建协程，那么应该首先执行一下Fiber::GetThis()操作，以初始化主函数协程
 */
Fiber::ptr Fiber::GetThis() {
    if(t_scheduler_fiber) {
        /**
         * 如果当前协程存在，则返回对应的智能指针。
         *  
         * t_fiber是裸指针，Fiber继承了enable_shared_form_this，故可获取外部管理它的智能指针
         * 原理是enable_shared_form_this中的一个私有成员 weak_ptr，当外部创建管理它的智能指针时，会将这个weak_this指向这个对象
         * 在类内部调用shared_from_this返回的是weak_this.lock()，即对应的智能指针。
         * 
         */
        return t_scheduler_fiber->shared_from_this();
    }
    // 不存在，代表主协程未创建，进行创建
    // std::make_shared<Fiber>()是错误的，make_shraed在std空间中，无法访问私有函数。Fiber()中有SetThis()
    Fiber::ptr main_fiber(new Fiber());
    // 确认是否创建，默认构造中调用了SetThis()
    SYLAR_ASSERT(t_scheduler_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_scheduler_fiber->shared_from_this();
}

void Fiber::SetThis(Fiber* fiber) {
    t_scheduler_fiber = fiber;
}

uint64_t Fiber::GetFiberId() {
    if(t_scheduler_fiber) {
        return t_scheduler_fiber->getId();
    }
    return 0;
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    try
    {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(const std::exception& e) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << e.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }
    catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }
    // 释放智能指针
    auto raw_ptr = cur.get();
    // 引用计数-1，主动释放Fiber::ptr。因为该Fiber::ptr局部变量存储在该协程的栈空间中，必须手动释放
    cur.reset();
    // 切回调度协程
    raw_ptr->swapOut();

    /**
     * @brief 为什么会出现"never reach"？在没有CallerMainFunc()之前，rootFiber使用的是swapOut()。
     * 犯了与swapIn()同样的错误，调度协程不能使用swapOut()，swapOut()与调度协程进行切换，相当于与自己切换，根本没离开。
     * 调度协程必须用back()与主协程切换才行。
     */
    SYLAR_ASSERT2(false, "never reach");
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    try
    {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(const std::exception& e) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << e.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }
    catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }
    // 释放智能指针
    auto raw_ptr = cur.get();
    // 引用计数-1，主动释放Fiber::ptr。因为该Fiber::ptr局部变量存储在该协程的栈空间中，必须手动释放
    cur.reset();
    // 切回主协程
    raw_ptr->back();

    SYLAR_ASSERT2(false, "never reach");
}

}