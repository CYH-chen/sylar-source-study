#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

volatile int count = 0;
// sylar::RWMutex s_mutex;
 sylar::Mutex s_mutex;

void fun1() {
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                            << " this.name: " << sylar::Thread::GetThis()->getName()
                            << " id: " << sylar::GetThreadId()
                            << " this.id: " << sylar::Thread::GetThis()->getId();
    for(int i = 0; i < 100000; ++i) {
        // 创建RAII对象进行加锁，局部变量在离开临界区后会自动析构释放锁
        // 比如此处：进入下一次循环之后自动调用析构函数解锁
        // sylar::RWMutex::WriteLock lock(s_mutex);
        // sylar::RWMutex::ReadLock lock(s_mutex);
        sylar::Mutex::Lock lock(s_mutex);
        count++;
    }
}

void fun2() {

}

int main(int argc, char* argv[]) {
    YAML::Node root = YAML::LoadFile("../bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);
    // 使用配置文件初始化
    // sylar::LogFormatter::ptr formatter = std::make_shared<sylar::LogFormatter>("%d%T%m%n");
    // g_logger->addAppender(std::make_shared<sylar::StdoutLogAppender>(formatter));

    SYLAR_LOG_INFO(g_logger) << "thread test begin.";
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i) {
        sylar::Thread::ptr thr = std::make_shared<sylar::Thread>(&fun1, "threadName_" + std::to_string(i));
        thrs.push_back(thr);
    }

    for(int i = 0; i < 5; ++i) {
        thrs[i]->join();
    }

    SYLAR_LOG_INFO(g_logger) << "thread test end.";
    SYLAR_LOG_INFO(g_logger) << "the count: " << count;
    return 0;
}


