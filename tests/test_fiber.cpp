#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    SYLAR_LOG_INFO(g_logger) << "child fiber run.";
    // 挂起
    sylar::Fiber::GetThis()->YieldToHold();
    SYLAR_LOG_INFO(g_logger) << "child fiber end.";
    // 必须要切回主协程,否则swap()后的都不执行了
    sylar::Fiber::GetThis()->swapOut();
}

// 此时fiber只会析构主协程，需要进一步修改
int main(int argc, char* argv[]) {
    // 必须先创建主协程
    sylar::Fiber::GetThis();
    SYLAR_LOG_INFO(g_logger) << "main begin.";
    // 创建子协程
    sylar::Fiber::ptr fiber = std::make_shared<sylar::Fiber>(run);
    // 切换到fiber
    fiber->swapIn();
    SYLAR_LOG_INFO(g_logger) << "main after swapIn.";
    fiber->swapIn();
    SYLAR_LOG_INFO(g_logger) << "main end.";
    return 0;
}