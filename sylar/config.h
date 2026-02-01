/**
 * @file config.h
 * @brief 配置模块
 * @version 0.1
 * @date 2026-01-13
 */
#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <concepts>
#include <functional>
#include "log.h"
#include "thread.h"

namespace sylar {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "") 
        :m_name(name) 
        ,m_description(description){
        // 转小写
        // ::代表全局命名空间，std::为标准命名空间，tolower在c++中有两个实现分别位于cctype和locale头文件中
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}

    const std::string& getName() const { return m_name;}
    const std::string& getDescription() const {return m_description;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    // 虚函数，map里的配置类才能调用他们的方法
    virtual std::string getTypeName() const = 0;
protected:
    std::string m_name;
    std::string m_description;
};

// F from_type; T to_type
// template<class F, class T, class Enable = void>
template<class F, class T>
class Lexical_cast {
public:
    // 从 F -> T 重载()运算符
    T operator()(const F& val) {
        return boost::lexical_cast<T>(val);
    }
};

// // 容器判定 C++ 20之前
// // vector / list 的判定
// template<class T>
// struct is_sequence_container : std::false_type {};
// template<class T, class Alloc>
// struct is_sequence_container<std::vector<T, Alloc>> : std::true_type {};
// template<class T, class Alloc>
// struct is_sequence_container<std::list<T, Alloc>> : std::true_type {};
// // set / unordered_set 的判定
// template<class T>
// struct is_set_container : std::false_type {};
// template<class Key, class Compare, class Alloc>
// struct is_set_container<std::set<Key, Compare, Alloc>> : std::true_type {};
// template<class Key, class Hash, class Eq, class Alloc>
// struct is_set_container<std::unordered_set<Key, Hash, Eq, Alloc>> : std::true_type {};
// // map / unordered_map 的判定
// template<class T>
// struct is_map_container : std::false_type {};
// template<class Key, class T, class Compare, class Alloc>
// struct is_map_container<std::map<Key, T, Compare, Alloc>> : std::true_type {};
// template<class Key, class T, class Hash, class Eq, class Alloc>
// struct is_map_container<std::unordered_map<Key, T, Hash, Eq, Alloc>> : std::true_type {};

// vector / list以及类似容器(去除 String )
template<class T>
concept SequenceContainer =
    requires(T c, typename T::value_type v) {
        { c.push_back(v) };
    }
    && (!std::same_as<std::remove_cvref_t<T>, std::string>);
// set / unordered_set 以及类似容器(去除 Map )
template<class T>
concept SetContainer =
    requires(T c, typename T::value_type v) {
        { c.insert(v) };
    }
    && (!requires { typename T::mapped_type; });
// map / unordered_map 以及类map容器
template<class T>
concept StringKeyMapContainer =
    requires {
        // 确认key和mapped的存在性
        typename T::key_type;
        typename T::mapped_type;
    }
    // 强制 key_type 必须是 std::string
    && std::same_as<typename T::key_type, std::string>
    // 像标准 map 一样使用
    && requires(T c, const std::string& k) {
        { c[k] }    -> std::same_as<typename T::mapped_type&>;
    };

// 统一 Sequence 和 Set
template<class T>
concept InsertableContainer =
    SequenceContainer<T> || SetContainer<T>;

template<InsertableContainer Container>
class Lexical_cast<std::string, Container> {
public:
    Container operator()(const std::string& str) {
        YAML::Node node = YAML::Load(str);
        Container container;
        // yaml不支持set类型的转换
        // Container container = node.as<Container>();
        std::stringstream ss;
        using ValueType = typename Container::value_type;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss.clear();
            ss << node[i];
            ValueType value =
                Lexical_cast<std::string, ValueType>()(ss.str());
            // 使用 if constexpr 进行区分
            if constexpr (SequenceContainer<Container>) {
                container.push_back(value);
            } else {
                container.insert(value);
            }
        }
        return container;
    }
};

template<InsertableContainer Container>
class Lexical_cast<Container, std::string> {
public:
    std::string operator()(const Container& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        using ValueType = typename Container::value_type;
        for (auto& i : v) {
            node.push_back(YAML::Load(Lexical_cast<ValueType, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<StringKeyMapContainer Container>
class Lexical_cast<std::string, Container> {
public:
    Container operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Container container;
        // // yaml原生支持map
        // 但map里不能包含不支持的类型，比如自定义类
        // Container container = node.as<Container>();
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it) {
            using MappedType = typename Container::mapped_type;
            ss.str("");
            ss.clear();
            ss << it->second;
            container.insert(std::make_pair(it->first.Scalar(),
                        Lexical_cast<std::string, MappedType>()(ss.str())));
        }
        return container;
    }
};

template<StringKeyMapContainer Container>
class Lexical_cast<Container, std::string> {
public:
    std::string operator()(const Container& v) {
        YAML::Node node(YAML::NodeType::Map);
        for(auto& i : v) {
            using MappedType = typename Container::mapped_type;
            node[i.first] = YAML::Load(Lexical_cast<MappedType, std::string>()(i.second));
        }
        // // yaml原生支持map
        // 但map里不能包含不支持的类型，比如自定义类
        // YAML::Node node(v);
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};



// 未使用 if constexpr 
// 使用concept + requires进行偏特化判定
// template<SequenceContainer Container>
// class Lexical_cast<std::string, Container> {
// public:
//     // 可递归使用，将复杂类型转化为复杂类型的数组，逐渐拆分
//     Container operator()(const std::string& str) {
//         YAML::Node node =  YAML::Load(str);
//         Container container;
//         std::stringstream ss;
//         // 其为数组 node.Type() == 3
//         for(size_t i = 0; i < node.size(); i++) {
//             using ValueType = typename Container::value_type;
//             ss.str("");
//             ss.clear();
//             ss << node[i];
//             // 再次使用模板将string转为T，若T为复杂类型，则是调用偏特化模板
//             // 类似递归调用，只不过这里是模板递归
//             container.push_back(Lexical_cast<std::string, ValueType>()(ss.str()));
//         }
//         return container;
//     }
// };
// template<SequenceContainer Container>
// class Lexical_cast<Container, std::string> {
// public:
//     std::string operator()(const Container& v) {
//         YAML::Node node(YAML::NodeType::Sequence);
//         for(auto& i : v) {
//             using ValueType = typename Container::value_type;
//             // 若T为复杂类型，则是调用偏特化模板
//             // 类似递归调用，只不过这里是模板递归
//             node.push_back(YAML::Load(Lexical_cast<ValueType, std::string>()(i)));
//         }
//         std::stringstream ss;
//         ss << node;
//         return ss.str();
//     }
// };
// template<SetContainer Container>
// class Lexical_cast<std::string, Container> {
// public:
//     // 可递归使用，将复杂类型转化为复杂类型的数组，逐渐拆分
//     Container operator()(const std::string& str) {
//         YAML::Node node =  YAML::Load(str);
//         Container container;
//         std::stringstream ss;
//         // 其为数组 node.Type() == 3
//         for(size_t i = 0; i < node.size(); i++) {
//             using ValueType = typename Container::value_type;
//             ss.str("");
//             ss.clear();
//             ss << node[i];
//             // 再次使用模板将string转为T，若T为复杂类型，则是调用偏特化模板
//             // 类似递归调用，只不过这里是模板递归
//             container.insert(Lexical_cast<std::string, ValueType>()(ss.str()));
//         }
//         return container;
//     }
// };
// template<SetContainer Container>
// class Lexical_cast<Container, std::string> {
// public:
//     std::string operator()(const Container& v) {
//         YAML::Node node(YAML::NodeType::Sequence);
//         for(auto& i : v) {
//             using ValueType = typename Container::value_type;
//             // 若T为复杂类型，则是调用偏特化模板
//             // 类似递归调用，只不过这里是模板递归
//             node.push_back(YAML::Load(Lexical_cast<ValueType, std::string>()(i)));
//         }
//         std::stringstream ss;
//         ss << node;
//         return ss.str();
//     }
// };



// FromStr T operator()(const std::string& str) | str -> T
// Tostr std::string operator()(const T&) | T -> str
// 后面两个使用默认值，且这个默认值是Lexical_cast的对应模板
template<class T, class FromStr = Lexical_cast<std::string, T>
                , class ToStr = Lexical_cast<T, std::string> >
class ConfigVar : public ConfigVarBase {
public:
    // 在类内部 ConfigVar 等价于 ConfigVar<T>
    typedef std::shared_ptr<ConfigVar> ptr;
    // 定义配置事件的接口，一个“当值发生变化时被调用的回调函数类型”
    // 观察者模式（函数式版本）
    typedef std::function<void (const T& old_value, const T& new_value) > on_change_cb;
    typedef RWMutex RWMutexType;

    ConfigVar(const std::string& name
            ,const T& default_val
            ,const std::string& decription = "") 
        :ConfigVarBase(name, decription)
        ,m_val(default_val) {

    }
    // T类型转字符串
    std::string toString() override {
        try {
            RWMutexType::ReadLock lock(m_mutex);
            // return boost::lexical_cast<std::string>(m_val);
            // 替换为统一接口
            // ToStr()(m_val)等价于Lexical_cast<T, std::string>(m_val)，是一个临时对象在调用()函数
            return ToStr()(m_val);
        }catch(std::exception& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    // 字符串转T类型
    bool fromString(const std::string& val) override {
        try {
            // m_val = boost::lexical_cast<T>(val);
            // 替换为统一接口
            setValue(FromStr()(val));
            return true;
        }catch(std::exception& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
                << e.what() << " convert: string to " << typeid(m_val).name()
                << " - string_val: " << val;
        }
        return false;
    }
    // 返回const引用
    const T& getValue() const { 
        RWMutexType::ReadLock lock(m_mutex);
        return m_val;
    }
    void setValue(const T& val) {
        // 局部锁
        { 
            // 回调函数的时间可能很长，可以用读锁
            RWMutexType::ReadLock lock(m_mutex);
            // 这里用了 == ，因此要求传入的 T 重载了 == 运算符
            if( val == m_val ) {
                return;
            }
            // 逐个通知
            for(auto& i : m_cbs) {
                i.second(m_val, val);
            }
        }
        // 修改时改为写锁
        RWMutexType::WriteLock lock(m_mutex);
        m_val = val;
    }
    // 返回T的类型名
    std::string getTypeName() const override { return typeid(T).name();}

    uint64_t addListener(on_change_cb cb) {
        RWMutexType::WriteLock lock(m_mutex);
        // s_fun_id转移到类成员变量，方便clear的时候清空
        // static uint64_t s_fun_id = 0;
        ++m_fun_id;
        m_cbs[m_fun_id] = cb;

        SYLAR_LOG_DEBUG(SYLAR_LOG_ROOT()) << getTypeName() << ": An listener has been added.";
        return m_fun_id;
    }

    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
        SYLAR_LOG_DEBUG(SYLAR_LOG_ROOT()) << getTypeName() << ": An listener has been erased.";
    }

    on_change_cb getListener(uint64_t key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
        m_fun_id = 0;
        SYLAR_LOG_DEBUG(SYLAR_LOG_ROOT()) << getTypeName() << ": All listeners have been cleared.";
    }
private:
    T m_val;
    /**
     * @brief 变更回调数组，通过key来确定function
     * 回调函数没有办法比较，即不能直接确定是否为同样的回调函数，固用map而不是用vector来存
     * uint64_t，要求唯一，使用hash确保唯一 
     */
    std::map<uint64_t, on_change_cb> m_cbs;
    /**
     * @brief 回调函数的key值
     * 
     */
    uint64_t m_fun_id = 0;
    // mutable突破const的限制
    mutable RWMutexType m_mutex;
};

// 管理类，类似LoggerMgr
class Config {
public:
    /**
     * @brief map 的 value 类型必须是“同一种类型”，所以只能用它们的共同基类 ConfigVarBase
     * type-erasure（类型擦除）设计。toString()、fromString()为纯虚函数
     * 在 Lookup<T> 时恢复强类型
     *      
     * */
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex RWMutexType;
    
    // 创建和查找函数
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
            const T& default_val, const std::string& description) {
        {
            // 查询时用读锁
            RWMutexType::ReadLock lock(GetMutex());
            // 调用时必须显式指定模板参数T，才能识别对应函数
            // auto temp = Lookup<T>(name);
            // 防止把“类型错误”当成了 name 不存在, 若dynamic_pointer_cast转换失败则是类型错误
            auto it = GetDatas().find(name);
            if(it != GetDatas().end()) {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
                if(tmp) {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return tmp;
                } else {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                            << typeid(T).name() << " real_type=" << it->second->getTypeName()
                            << " " << it->second->toString();
                    return nullptr;
                }
            }
        }

        // find_first_not_of()若找到，则返回位置索引；若找不到，即全部字符都合法，则返回std::string::npos
        // 构造时已强制转小写
        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")
                != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
        // 没有错误，则创建
        // typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_val, description));
        typename ConfigVar<T>::ptr v = std::make_shared<ConfigVar<T> >(name, default_val, description);
        // 修改时用写锁
        RWMutexType::WriteLock lock(GetMutex());
        GetDatas()[name] = v;
        return v;
    }

    
    // 查找函数
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end()) {
            return nullptr;
        }
        // it->second 是 shared_ptr<ConfigVarBase>
        // 必须进行 “显示转换” 才能转换成ConfigVar<T>，不能隐式转换
        // 类型错误不会被“偷偷转换”，会返回nullptr
        typename ConfigVar<T>::ptr ret = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
        if(!ret) {
            // 类型转换错误，报错
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name:" << name << "already exists and the content will be revised.";
        }
        return ret;
    }

    /**
     * @brief 从YAML的Node节点中读取所有配置项
     * 
     * @param node YAML的Node节点
     */
    static void LoadFromYaml(const YAML::Node& node);
    /**
     * @brief 查找配置参数
     * 
     * @param name 配置项名
     * @return ConfigVarBase::ptr 配置基类
     */
    static ConfigVarBase::ptr LookupBase(const std::string& name);
    /**
     * @brief 观察者模式，输出ConfigVarMap，查看其内容
     * 
     * @param cb 输出输出ConfigVarMap的方法，从外部导入，与数据结构无关
     */
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);
private:
    // /**
    //  * @brief 统一存储所有配置项的容器（这里使用inline，不需要在.cpp中再定义）
    //  * 类内的 static 数据成员：类内声明 ≠ 实体存在
    //  * 必须在类外提供一次定义（除非是 inline 或 constexpr）
    //  */
    // inline static ConfigVarMap s_datas;

    /**
     * @brief 获得静态变量配置项数据。
     * 直接使用静态变量有风险，有可能使用Lookup的时候s-datas还未初始化，但构造配置项的变量先初始化的情况下会出问题。
     * 因为静态变量初始化顺序是未知的，而使用静态函数返回静态函数内部的静态变量可以防止这个问题。
     * 静态函数在第一次调用时才初始化，只要调用就会进行初始化。
     * 
     * @return ConfigVarMap& 
     */
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }
    static RWMutexType& GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};

}

#endif