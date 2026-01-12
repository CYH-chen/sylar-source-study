/**
 * @file singleton.h
 * @brief 单例
 * @version 0.1
 * @date 2026-01-13
 */

#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>

namespace sylar {

template<class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 返回单例地址
     * 
     * @return T* 
     */
    static T* GetInstance() {
        static T v;
        return &v;
    }
};

template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}
#endif