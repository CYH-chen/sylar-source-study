/**
 * @file test_config.cpp
 * @brief 配置系统的测试文件
 * @version 0.1
 * @date 2026-01-14

 */
#include <iostream>
#include "../sylar/config.h"
#include "../sylar/log.h"
// 测试yaml的路径
const std::string testPath = "../bin/conf/test.yml";
const std::string logPath = "../bin/conf/log.yml";

// 初始化  /  查找对应字段
sylar::ConfigVar<int>::ptr g_int_value_config =
    // 显示指定<int>，也可不指定可自动推断
    sylar::Config::Lookup<int>("system.port", (int)8080, "system port");

// 重复赋值测试
// sylar::ConfigVar<float>::ptr g_muiti_error_value_config =
//     sylar::Config::Lookup("system.port", (float)10.2f, "system value");

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
    YAML::Node node = YAML::LoadFile(testPath);

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

    YAML::Node node = YAML::LoadFile(testPath);
    sylar::Config::LoadFromYaml(node);

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


class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace sylar {

// 全特化对自定义类进行序列化和反序列化
template<>
class Lexical_cast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class Lexical_cast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}
// 自定义类型测试
sylar::ConfigVar<Person>::ptr g_person =
    sylar::Config::Lookup("class.person", Person(), "system person");
// Map里套自定义类型测试
sylar::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    sylar::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

// 先加了一层vector，再在vector里加Map，多嵌套了两层
sylar::ConfigVar<std::map<std::string, std::vector<std::map<std::string, Person>> > >::ptr g_person_vec_map =
    sylar::Config::Lookup("class.vec_map", std::map<std::string, std::vector<std::map<std::string, Person>> >(), "system person");

void test_class() {

    // 在value更新时会调用，before后，after前，在sylar::Config::LoadFromYaml(root)的时候
    g_person->addListener([](const Person& old_value, const Person& new_value){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << old_value.toString()
                    << " new_value=" << new_value.toString();
        });

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();
    
#define XX_PM(g_var, prefix) \
    { \
        auto m = g_var->getValue(); \
        for(auto& i : m) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<  prefix << ": size=" << m.size() << " - " << g_var->toString(); \
    }
    XX_PM(g_person_map, "class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: map_vec_person - " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile(testPath);
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: map_vec_person - " << g_person_vec_map->toString();
#undef XX
}

void test_log() {
    static sylar::Logger::ptr system_log =SYLAR_LOG_NAME("system");
    system_log->addAppender(std::make_shared<sylar::StdoutLogAppender>());
    SYLAR_LOG_INFO(system_log) << "hello system" << std::endl;
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << SYLAR_LOG_TOYAMLSTRING();
    YAML::Node root = YAML::LoadFile(logPath);
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_WARN(SYLAR_LOG_ROOT()) << "root after: " << SYLAR_LOG_ROOT()->toYamlString();
    SYLAR_LOG_INFO(SYLAR_LOG_NAME("system")) << "system after: " << SYLAR_LOG_NAME("system")->toYamlString();

    SYLAR_LOG_INFO(system_log) << "system after: hello system" << std::endl;
    std::cout << "====================\nSYLAR_LOG_TOYAMLSTRING():\n" << SYLAR_LOG_TOYAMLSTRING() << std::endl;
}


int main(int argc, char* argv[]) {
    // test_yaml();

    // test_config();

    // test_class();

    test_log();
    return 0;
}