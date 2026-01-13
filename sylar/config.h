/**
 * @file config.h
 * @brief 配置系统
 * @version 0.1
 * @date 2026-01-13
 */
#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "log.h"

namespace sylar {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "") 
        :m_name(name) 
        ,m_description(description){
    }
    virtual ~ConfigVarBase() {}

    const std::string& getName() const { return m_name;}
    const std::string& getDescription() const {return m_description;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
protected:
    std::string m_name;
    std::string m_description;
};

template<class T>
class ConfigVar : public ConfigVarBase {
public:
    // 在类内部 ConfigVar 等价于 ConfigVar<T>
    typedef std::shared_ptr<ConfigVar> ptr;
    ConfigVar(const std::string& name
            ,const T& default_val
            ,const std::string& decription = "") 
        :ConfigVarBase(name, decription)
        ,m_val(default_val) {

    }
    // T类型转字符串
    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(m_val);
        }catch(std::exception& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    // 字符串转T类型
    bool fromString(const std::string& val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
            return true;
        }catch(std::exception& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
                << e.what() << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }
    // 返回const引用
    const T& getVal() const {return m_val;}
    void setVal(const T& val) { m_val = val;}
private:
    T m_val;
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
    
    // 创建和查找函数
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
            const T& default_val, const std::string& description) {
        // 调用时必须显式指定模板参数T，才能识别对应函数
        auto temp = Lookup<T>(name);
        // 如果是nullptr代表没有，或者类型错误
        if(temp) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name:" << name << " exists";
            return temp;
        }

        // find_first_not_of()若找到，则返回位置索引；若找不到，即全部字符都合法，则返回std::string::npos
        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")
                != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
        // 没有错误，则创建
        // typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_val, description));
        typename ConfigVar<T>::ptr v = std::make_shared<ConfigVar<T> >(name, default_val, description);
        s_datas[name] = v;
        return v;
    }

    
    // 查找函数
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        auto it = s_datas.find(name);
        if(it == s_datas.end()) {
            return nullptr;
        }
        // it->second 是 shared_ptr<ConfigVarBase>
        // 必须进行 “显示转换” 才能转换成ConfigVar<T>，不能隐式转换
        // 类型错误不会被“偷偷转换”，会返回nullptr
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }
private:
    /**
     * @brief 统一存储所有配置项的容器（这里使用inline，不需要在.cpp中再定义）
     * 类内的 static 数据成员：类内声明 ≠ 实体存在
     * 必须在类外提供一次定义（除非是 inline 或 constexpr）
     */
    inline static ConfigVarMap s_datas;
};

}




#endif