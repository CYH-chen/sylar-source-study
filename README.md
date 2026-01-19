- [sylar-source-study](#sylar-source-study)
- [前言](#前言)
- [依赖安装](#依赖安装)
- [改动](#改动)
  - [日志系统](#日志系统)
    - [class](#class)
    - [LoggerManager::getLogger](#loggermanagergetlogger)
  - [配置系统](#配置系统)
    - [条件模板特化](#条件模板特化)
    - [`concept + requires` + `if constexpr`](#concept--requires--if-constexpr)

# sylar-source-study
参考项目：[sylar C++高性能分布式服务器框架](https://github.com/sylar-yin/sylar)
项目作者：[sylar](https://github.com/sylar-yin)

此外 sylar 有在b站发布教学视频：[\[C++高级教程\]从零开始开发服务器框架(sylar)](https://www.bilibili.com/video/av53602631/?from=www.sylar.top "")

其他参考项目：[sylar-from-scratch 从零开始重写sylar C++高性能分布式服务器框架](https://github.com/zhongluqiang/sylar-from-scratch)

# 前言
基本上遇到有复杂逻辑的地方都写了便于理解的注释。此外对于linux c++开发我自己也仍在学习中，如有错误还请多多包含。

# 依赖安装
以下命令适用Linux系统,使用`dnf`包管理器安装：
```shell
# 安装boost库：
sudo dnf install boost-devel
# 安装yaml-cpp库：
sudo dnf install yaml-cpp-devel
......
```
Tips：`yaml-cpp`和`yaml-cpp-devel`是两个独立的包，前者仅提供**运行时库**，后者提供开发所需的**头文件和库文件**。安装`yaml-cpp-devel`会自动安装`yaml-cpp`，`boost`同理。

# 改动
尝试使用了一些C++11之后的语法进行改写。

以下列出一些**改动（部分）**：
## 日志系统
### class
修改了一些类中的成员变量和函数行为，使得在我的视角下类的关系更为清晰。比如`formatter`只和`appender`绑定，`logger`只负责日志级别过滤和路由。
### LoggerManager::getLogger
不使用root对代替新注册的logger进行输出，而是直接不管，新创建的logger没有appender，无输出地。
## 配置系统
*  > 注意：yaml库原生就支持大部分类型转换，但是`set`、`unordered_set`以及**自定义类型**是不支持的。因此也可以自己补 `YAML::convert< std::set<T> >`，然后用yaml库的`node.as< >()`进行转换。不过下述的模板类的结构是不变的，变的仅仅是对具体数据的处理细节。
### 条件模板特化
在[ sylar 的配置系统 config.h ](https://github.com/sylar-yin/sylar/blob/master/sylar/config.h)中，使用了**模板偏特化**来处理不同类型配置项的**序列化和反序列化**。对于普通类型（`int`,`float`等）使用主模板中的`boost::lexical_cast<T>(val)`;而对于复杂类型(`vector`,`set`等)使用模板偏特化进行特殊处理。
可以发现`vector`,`list`使用了共同的`push_back`操作；同理`set`,`unordered_set`使用了相同的`insert`操作。在这几个类型中，重载的函数体几乎一模一样，仅仅是容器不同，因此在这里考虑使用**条件模板特化**。
1. 主模板中额外添加一个默认参数`class Enable = void`
2. 进行自定义**类型萃取**，将类型进行分类，以下展示`vector`,`list`的归类
   ```cpp
    // vector / list 的判定
    template<class T>
    struct is_sequence_container : std::false_type {};
    template<class T, class Alloc>
    struct is_sequence_container<std::vector<T, Alloc>> : std::true_type {};
    template<class T, class Alloc>
    struct is_sequence_container<std::list<T, Alloc>> : std::true_type {};
   ```
3. `std::enable_if_t`利用**SFINAE**机制控制函数模板是否可用

   其中使用`Container::value_type`获取容器中的“类型别名”，便于进行配置项的嵌套转换
   ```cpp
    template<class Container>
    class Lexical_cast<
        std::string,
        Container,
        std::enable_if_t<is_sequence_container<Container>::value>
    > {
    public:
        Container operator()(const std::string& str) {
            ......
            // Container::value_type 是指这个容器中的“元素类型”
            using ValueType = typename Container::value_type;
            ......
        }
    };
   ```
    通过**条件模板特化**可以减少重复代码，由原来12个模板偏特化减少至6个；并且修改起来也较为方便(~~虽然后面也用不着改~~)。
### `concept + requires` + `if constexpr`
一开始我用的是`std::enable_if_t`，然后在看新特性的时候发现了`concept + requires`(C++ 20)以及 `if constexpr`(C++ 17)，觉得可以进行进一步简化。
1. 在上述的基础上，先写`Concept`约束
   ```cpp
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
   ```
2. 将约束应用到偏特化模板
   ```cpp
    // 使用concept + requires进行偏特化判定
    template<SequenceContainer Container>
    class Lexical_cast<std::string, Container> {
    public:
        ......
    }
   ```
    使用`Concept`的可读性显然优于`std::enable_if_t`

3. 使用`if constexpr`对逻辑相似的模板进行合并
   
   在这里`Sequence`和`Set`的逻辑显然高度相似，故可将二者进行合并处理。
   1. 需要一个统一的`Concept`约束
        ```cpp
        // 统一 Sequence 和 Set
        template<class T>
        concept InsertableContainer =
            SequenceContainer<T> || SetContainer<T>;
        ```
   2. 进行合并

        以下仅展示 Container -> std::String
        ```cpp
        template<InsertableContainer Container>
        class Lexical_cast<std::string, Container> {
        public:
            Container operator()(const std::string& str) {
                ......
                using ValueType = typename Container::value_type;
                ......
                ValueType value = Lexical_cast<std::string, ValueType>()(ss.str());
                // 使用 if constexpr 进行区分
                if constexpr (SequenceContainer<Container>) {
                    container.push_back(value);
                } else {
                    container.insert(value);
                }
                ......
            }
        };
        ```
    **通过应用新特性，将转化函数进一步精简，由 12个 -> 6个 -> 4个；甚至在精简的同时还提升了可读性。**
