/**
 * @file test_config.cpp
 * @brief 配置系统的测试文件
 * @version 0.1
 * @date 2026-01-14

 */
#include <iostream>
#include "../sylar/config.h"
#include "../sylar/log.h"
#include <yaml-cpp/yaml.h>

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup<int>("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");

void test_yaml() {
    YAML::Node node = YAML::LoadFile("../bin/conf/log.yml");

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << node;
}

int main(int argc, char* argv[]) {
    
    // 基本类型的config转换
    SYLAR_LOG_DEBUG(SYLAR_LOG_ROOT()) << g_int_value_config->getVal();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_float_value_config->toString();
    
    test_yaml();
    return 0;
}