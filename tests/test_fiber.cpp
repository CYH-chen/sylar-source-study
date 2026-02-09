#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    SYLAR_LOG_INFO(g_logger) << "child fiber run.";
    // 挂起
    sylar::Fiber::GetThis()->YieldToHold();
    SYLAR_LOG_INFO(g_logger) << "child fiber end.";
}

void test_fiber() {
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
}

int main(int argc, char* argv[]) {
    sylar::Thread::SetName("main");

    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        // 函数名在需要时，会自动退化为函数指针，所以 test_fiber 和 &test_fiber 在作为函数对象参数时等价。
        thrs.push_back(std::make_shared<sylar::Thread>(&test_fiber, "thread-" + std::to_string(i)));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}