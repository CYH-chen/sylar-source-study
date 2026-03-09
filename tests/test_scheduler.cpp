#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

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
     */
    sylar::Scheduler sc(2, true, "sheduler");
    sc.start();
    SYLAR_LOG_INFO(g_logger) << "scheduler have started.";
    sc.stop();

    return 0;
}