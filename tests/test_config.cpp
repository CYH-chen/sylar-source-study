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

sylar::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config =
    sylar::Config::Lookup("system.int_vec", std::vector<int>{1,2,3}, "system int vec");

sylar::ConfigVar<std::list<int> >::ptr g_int_list_value_config =
    sylar::Config::Lookup("system.int_list", std::list<int>{4,5}, "system int list");

sylar::ConfigVar<std::set<int> >::ptr g_int_set_value_config =
    sylar::Config::Lookup("system.int_set", std::set<int>{724,888,724}, "system int set");

sylar::ConfigVar<std::unordered_set<int> >::ptr g_int_unordered_set_value_config =
    sylar::Config::Lookup("system.int_unordered_set", std::unordered_set<int>{101,777,101}, "system int unordered_set");

sylar::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config =
    sylar::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}}, "system str int map");

sylar::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_unordered_map_value_config =
    sylar::Config::Lookup("system.str_int_unordered_map", std::unordered_map<std::string, int>{{"uk",3}}, "system str int unordered_map");

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
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();
// 相邻的字符串字面量在编译期会自动拼接成一个字符串，字符串拼接规则 只适用于字符串字面量
#define XX(set_config_func, name, when) \
    { \
        auto& it =  set_config_func->getValue(); \
        for(auto& i : it) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #when " " #name ": "<< i; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #when " " #name " yaml: " << set_config_func->toString(); \
    }

#define XX_M(set_config_func, name, when) \
    { \
        auto& it = set_config_func->getValue(); \
        for(auto& i : it) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #when " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #when " " #name " yaml: " << set_config_func->toString(); \
    }
        
    XX(g_int_vec_value_config, int_vector, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_unordered_set_value_config, int_unordered_set, before);
    XX_M(g_str_int_map_value_config, int_map, before);
    XX_M(g_str_int_unordered_map_value_config, int_unordered_map, before);

    YAML::Node node = YAML::LoadFile("../bin/conf/log.yml");
    sylar::Config::LoadFromFile(node);

    XX(g_int_vec_value_config, int_vector, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_unordered_set_value_config, int_unordered_set, after);
    XX_M(g_str_int_map_value_config, int_map, after);
    XX_M(g_str_int_unordered_map_value_config, int_unordered_map, after);

    // 基本类型的config转换
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

#undef XX
}

int main(int argc, char* argv[]) {
    // test_yaml();

    test_config();
    return 0;
}