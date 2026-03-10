#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test_fiber().";
    static int count = 5;
    // 不能只有“count--”，在多个相同任务中会出现小于0再执行这个任务的情况.
    // if(count-- > 0) {
    //     sylar::Scheduler::GetThis()->schedule(test_fiber);

    // }
    // 在这里schedule的话，非指定线程会在run循环中疯狂tickle

    sleep(1);

    if(count-- > 0) {
        sylar::Scheduler::GetThis()->schedule(test_fiber, sylar::GetThreadId());
    }
}

int main(int argc, char* argv[]) {
    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    /**
     * @brief 运行过程：
     * 1、日志模块添加回调函数，配置更新
     * 2、Scheduler类初始化，默认use_caller == true。
     *    use_caller == true 先尝试创建主协程，再创建单独的调度协程。
     *    use_caller == false 其主线程就是调度协程。
     *    随后创建idle协程，完成初始化。
     *    部分在构造函数完成，部分在run()开头完成。
     * 3、进入循环，如果队列为空，直接进入idle协程，随后idle协程运行完毕。
     * 4、进入退出阶段top()。tickle()所有线程，线程运行结束，开始析构。
     * 
     * 
     * 
     * 1是携程调度器所在，2是start的idle，3是执行test任务的协程
     */
    SYLAR_LOG_INFO(g_logger) << "main start";

    sylar::Scheduler sc(2, true, "sheduler");
    // start()后，由调度协程进行任务分派，主协程需要等调度协程结束才能回来
    sc.start();
    SYLAR_LOG_INFO(g_logger) << "sc.start() end.";

    // // 开一个线程往里面加任务
    // sylar::Thread thread([&sc](){
    //     sc.schedule(test_fiber);
    // }, "add");
    // thread.join();
    sc.schedule(test_fiber);
    sc.stop();

    SYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}