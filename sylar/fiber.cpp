/**
 * @file fiber.cpp
 * @brief 协程模块实现
 * @version 0.1
 * @date 2026-02-04
 * 
 */
#include <atomic>
#include "fiber.h"
#include "config.h"
#include "macor.h"

namespace sylar {

static std::atomic<uint64_t> s_fiber_id { 0 };
static std::atomic<uint64_t> s_fiber_count { 0 };

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
}

Fiber::~Fiber() {
    --s_fiber_count;
    // 主协程没有栈空间 m_stack == nullptr
    if(m_stack) {
        // 有栈的情况下，协程状态要么状态是刚初始化，要么是结束了
        SYLAR_ASSERT(m_state == INIT 
                    || m_state == TERM);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else {
        // 此时为主协程，没有callback函数且状态为EXEC
        SYLAR_ASSERT(!m_cb && m_state == EXEC);
        // 比较是否是主协程
        Fiber* cur =  t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
}

void Fiber::reset(std::function<void()> cb) {

}

void Fiber::swapIn() {

}

void Fiber::swapOut() {

}

Fiber::ptr Fiber::GetThis() {

}

void Fiber::SetThis(Fiber* fiber) {

}

void Fiber:: YieldToReady() {

}

void Fiber::YieldToHold() {

}

uint64_t Fiber::TotalFibers() {

}

void Fiber::MainFunc() {

}

}