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

namespace sylar {

static std::atomic<uint64_t> s_fiber_id { 0 };
static std::atomic<uint64_t> s_fiber_count { 0 };

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 线程的当前协程
static thread_local Fiber* t_fiber = nullptr;
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
    ++s_fiber_count;
    m_state = EXEC;
    SetThis(this);

    // 获取上下文
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize) 
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
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    // 指定入口函数，真正切换用swapcontext
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

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
        Fiber* cur =  t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
}

void Fiber::reset(std::function<void()> cb) {
    // 有栈且状态对的协程才能重置
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == INIT || m_state == EXCEPT || m_state == TERM);
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
    // 将当前状态保存到old_context中，切换到new_context
    if(swapcontext(&(t_threadFiber->m_ctx), &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext error");
    }
}

void Fiber::swapOut() {
    // 切回主协程
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &(t_threadFiber->m_ctx))) {
        SYLAR_ASSERT2(false, "swapcontext error");
    }
}

Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        /**
         * 如果当前协程存在，则返回对应的智能指针。
         *  
         * t_fiber是裸指针，Fiber继承了enable_shared_form_this，故可获取外部管理它的智能指针
         * 原理是enable_shared_form_this中的一个私有成员 weak_ptr，当外部创建管理它的智能指针时，会将这个weak_this指向这个对象
         * 在类内部调用shared_from_this返回的是weak_this.lock()，即对应的智能指针。
         * 
         */
        return t_fiber->shared_from_this();
    }
    // 不存在，代表主协程未创建，进行创建
    // std::make_shared<Fiber>()是错误的，make_shraed在std空间中，无法访问私有函数
    Fiber::ptr main_fiber(new Fiber());
    // 确认是否创建，默认构造中调用了SetThis()
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::SetThis(Fiber* fiber) {
    t_fiber = fiber;
}

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

void Fiber:: YieldToReady() {
    Fiber::ptr cur = GetThis();
    // 禁止在主协程调用该方法
    SYLAR_ASSERT(cur != t_threadFiber);
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    // 禁止在主协程调用该方法
    SYLAR_ASSERT(cur != t_threadFiber);
    cur->m_state = HOLD;
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
    
}

}