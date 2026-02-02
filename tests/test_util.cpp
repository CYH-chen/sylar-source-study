#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert() {
    // skip设置为2，跳过前两层的Backtrace调用
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10, 2, "       ");
    // SYLAR_ASSERT(false);
    SYLAR_ASSERT2(0 == 1, "test ASSERT2");
}

int main(int argc, char* argv[]) {
    test_assert();
    return 0;
}
