/**
 * @file scheduler.cpp
 * @brief 协程调度器模块实现
 * @version 0.1
 * @date 2026-02-12
 * 
 */
#include "scheduler.h"
#include "log.h"
#include "macor.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar {

static thread_local Scheduler* t_scheduler = nullptr;
// 执行Scheduler::run的协程，可称为调度协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
    :m_name(name) {
    SYLAR_ASSERT(threads > 0);

    if(use_caller) {
        // 先尝试创建主协程
        Fiber::GetThis();
        // 去除use_caller
        --threads;
        // 不能已存在Scheduler
        SYLAR_ASSERT(GetThis() == nullptr);
        // caller Thread也会被scheduler调度
        t_scheduler = this;
        /**
         * @brief 运行 std::bind()生成一个可调用对象（callable），相当于this->run().因为run需要通过this调用
         * @attention  main fiber 不能随便切换。
         * 如果调度器直接在 main fiber 运行：
         *    main fiber
         *    ↓
         *    Scheduler::run()
         *    ↓
         *    fiber swap
         * 调度结束后 无法返回调用 start() 的地方。
         * 因此使用一个调度协程rootFiber来执行run()，mainFiber仍进行协程切换任务。
         */
        m_rootFiber = std::make_shared<Fiber>(std::bind(&Scheduler::run, this), 0, true);
        Thread::SetName(m_name);
        /**
         * @brief 此处的调度协程就不是mainFiber了，而是额外的一个专门用于任务调度的协程。
         * 只有caller线程才会拥有，目的是为了不干扰caller线程的mainFiber的同时，能够在Scheduler中实现任务调度。
         * 非caller线程的t_scheduler_fiber就是mainFiber。
         */
        t_scheduler_fiber = m_rootFiber.get();
        m_rootThreadId = GetThreadId();
        m_threadIds.push_back(m_rootThreadId);
    }
    else {
        // 没有use_caller
        m_rootThreadId = -1;
        // 这里t_scheduler是nullptr
    }
    m_threadCount = threads;
}
Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    // 先启动线程池
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        // stopping为false，已经启动了
        return;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        // 创建Thread对象，全都运行Scheduler::run方法
        m_threads[i] = std::make_shared<Thread>(std::bind(&Scheduler::run, this),
                                    m_name + "_" + std::to_string(i));
        // 由于Thread的创建用了信号量，因此创建完毕后一定完成了参数初始化
        m_threadIds.push_back(m_threads[i]->getId());
    }
    // 主动解锁，防止死锁
    lock.unlock();
    if(m_rootFiber) {
        // caller线程切入调度协程，执行run()
        // 这里如果swapIn内是与调度协程切换，而m_rootFiber本身就是调度协程，相当于与自身切换，状态仍是EXEC就出来了。
        // 调度协程禁止使用swapIn()
        m_rootFiber->call();
        /**
         * @attention 这里使用独立的调度协程进行切入，需要修改fiber.h中的主协程获取方法。
         * 因为此时m_rootFiber就充当着主协程的作用，因此caller线程swapOut时必须与m_rootFiber进行切换。
         * 
         * 跑飞的原因：rootFiber先swapIn;而idleFiber再次swapIn，覆盖了rootFiber的旧上下文。
         * 导致rootFiber返回时，回到的是rootFiber的上下文，再也回不到main()了。
         * 此处使用swapIn()，而在进入的run()中又会有idle的swapIn()。一定会跑飞！！
         * 
         */
        SYLAR_LOG_DEBUG(g_logger) << "m_rootFiber回归";
    }
}

// 核心思想：等待所有任务执行完毕后stop
void Scheduler::stop() {
    m_autoStop = true;
    // 当线程池中仅有一个use_caller线程，并且其为Term或INIT状态
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;
        // 子类清理任务
        if(stopping()) {
            return;
        }
    }
    // 安全检查
    if(m_rootThreadId != -1) {
        // use_caller == true
        // 调用 Scheduler 的线程也参与调度，故GetThis会等于this
        SYLAR_ASSERT(GetThis() == this);
    } else {
        // caller thread 不参与调度
        // 故caller thread 没有 Scheduler，GetThis()会返回nullptr
        SYLAR_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    // 有多少个线程就唤醒多少次，唤醒完也就结束了
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    // 调度协程存在，要额外唤醒一次
    if(m_rootFiber) {
        tickle();
    }

    // 子类清理任务
    if(stopping()) {
        return;
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    setThis();
    if(GetThreadId() != m_rootThreadId) {
        /**
         * @brief 此时非use_caller线程，要对t_fiber进行初始化；use_call线程已经初始化了一个调度协程rootFiber执行run()。
         * 注意，此刻进行Fiber::GetThis()，获取的就是mainFiber。
         * 因为只有mainFiber创建之后，再创建子协程执行任务
         * 
         */
        t_scheduler_fiber = Fiber::GetThis().get();
    }
    // idle协程创建
    Fiber::ptr idleFiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle, this));
    // 用于执行回调函数的协程
    Fiber::ptr cbFiber;

    Task task;
    while(true) {
        // 循环一开始进行重置
        task.reset();
        bool tickle_me = false;
        {
            // 从任务的消息队列中选择一个任务
            MutexType::Lock lock(m_mutex);
            auto it = m_tasks.begin();
            while (it != m_tasks.end())
            {
                if(it->thread != -1 && it->thread != GetThreadId()) {
                    // 这是指定了线程号的协程，且当前抢到执行权的线程不是目标，跳过
                    ++it;
                    // 标记一下需要通知其他线程进行调度
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    // 协程正在运行（有可能当协程要yield前会将其加入协程的消息队列）
                    ++it;
                    continue;
                }
                // 取出任务并将其移出任务队列
                task = *it;
                ++m_activeThreadCount;
                m_tasks.erase(it++);
                break;
            }
            // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
            tickle_me |= (it != m_tasks.end());
        }

        if(tickle_me) {
            tickle();
        }
        
        if(task.fiber && (task.fiber->getState() != Fiber::TERM
                    && task.fiber->getState() != Fiber::EXCEPT)) {
            // 执行协程
            task.fiber->swapIn();
            --m_activeThreadCount;
            // 协程要自我进行管理，即调用yield()前，需要主动加入任务队列
            // if(task.fiber->getState() == Fiber::READY) {
            //     // 协程执行完后仍然为READY，说明发生了yield，需要重新加入任务队列
            //     schedule(std::move(task.fiber));
            // }
            task.reset();
        }
        else if(task.cb) {
            if(cbFiber) {
                // 复用Fiber，减少重新分配栈空间的开销
                cbFiber->reset(std::move(task.cb));
            } else {
                // 基于ft.cb创建一个协程
                cbFiber = std::make_shared<Fiber>(task.cb);
            }
            task.reset();

            // 执行协程
            cbFiber->swapIn();
            --m_activeThreadCount;
            
            // 协程要自我进行管理，即调用yield()前，需要主动加入任务队列
            // if(cbFiber->getState() == Fiber::READY) {
            //     // 协程执行完后仍然为READY，说明发生了yield，需要重新加入任务队列
            //     schedule(std::move(cbFiber));
            //     // cbFiber置空
            //     cbFiber.reset();
            // } 
            if(cbFiber->getState() == Fiber::EXCEPT || cbFiber->getState() == Fiber::TERM) {
                // 如果协程运行结束了。清除cb，但是不析构对象，方便下次使用
                cbFiber->reset(nullptr);
            } else {
                // 协程未结束，不能重新利用
                cbFiber.reset();
            }
        }
        else {
            // 进到这个分支情况一定是任务队列空了，调度idle协程即可

            if(idleFiber->getState() == Fiber::TERM) {
                // 如果ilde协程结束了
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idleFiber->swapIn();
            --m_idleThreadCount;
        }
        
    }
}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_tasks.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    // 此处会无限循环，如果没有外部调用stop()的话
    while(!stopping()) {
        sylar::Fiber::YieldToReady();
        // SYLAR_LOG_INFO(g_logger) << "YieldToReady";
    }
}

}