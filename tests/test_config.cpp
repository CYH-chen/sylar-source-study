/**
 * @file test_config.cpp
 * @brief 配置系统的测试文件
 * @version 0.1
 * @date 2026-01-14

 */
#include <iostream>
#include "../sylar/config.h"
#include "../sylar/log.h"

// 初始化  /  查找对应字段
sylar::ConfigVar<int>::ptr g_int_value_config =
    // 显示指定<int>，也可不指定可自动推断
    sylar::Config::Lookup<int>("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");

/**
 * @brief 递归遍历一个 YAML::Node，
 * 按层级结构把 YAML 内容“打印”到日志中，
 * 并且把节点类型和当前层级一起输出。
 * 
 * 用 level * 4 个空格表示层级关系
 * 
 * @param node 节点
 * @param level 层级
 */
void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node node = YAML::LoadFile("../bin/conf/log.yml");

    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << node;
    print_yaml(node, 0);
}

void test_config() {
    // 基本类型的config转换
    SYLAR_LOG_DEBUG(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getVal();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();
    
    YAML::Node node = YAML::LoadFile("../bin/conf/log.yml");
    sylar::Config::LoadFromFile(node);

    // 基本类型的config转换
    SYLAR_LOG_DEBUG(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getVal();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();
    
}

int main(int argc, char* argv[]) {
    test_yaml();

    test_config();
    return 0;
}