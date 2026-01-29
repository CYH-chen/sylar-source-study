/**
 * @file config.cpp
 * @brief 配置模块实现
 * @version 0.1
 * @date 2026-01-14
 */
#include "config.h"


namespace sylar {
// 类中使用了inline static，无需再次定义
// Config::ConfigVarMap Config::s_datas;

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

/**
 * 要考虑层级： "A.B", 10 转变为
 * A:
 *    B:10
 */
/**
 * @brief 把 YAML 树 flatten 成一张「完整配置路径 → YAML::Node」的表;
 * 
 * 有些配置 本身就是一个 Map、有些配置 需要整体覆盖
 * 因此，只对 Map 递归，Scalar / Sequence 不展开，Sequence 会整体作为一个 Node
 * 
 * @param prefix 前缀
 * @param node 当前处理节点
 * @param output 输出pair对的列表
 */
static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node> >& output) {
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
            != std::string::npos) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));

    if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            // 三目运算符而不是if-else
            // Scalar() 取出字符串
            ListAllMember(prefix.empty() ? it->first.Scalar()
                    : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

/**
 * @brief 按 key 找到已注册的 ConfigVar，然后把 YAML 节点转成字符串，写进去
 * 
 * @param node 
 */
void Config::LoadFromYaml(const YAML::Node& node) {
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;
    ListAllMember("", node, all_nodes);

    for(auto& i : all_nodes) {
        std::string key = i.first;
        if(key.empty()) {
            // 跳过根节点
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        //这里不会新建 ConfigVar，只有提前注册过的配置，才会被 YAML 覆盖
        ConfigVarBase::ptr var = LookupBase(key);
        // 只处理已注册的配置项
        if(var) {
            if(i.second.IsScalar()) {
                // 
                var->fromString(i.second.Scalar());
            } else {
                // 特殊项特殊处理
                std::stringstream ss;
                // Node 重新 dump 成 YAML 文本（yaml-cpp 已对<<重载）
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}
}