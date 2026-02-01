cmake_minimum_required(VERSION 3.0)
project(sylar-source-study)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS}-rdynamic -O0 -fPIC -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 在配置系统log.h中用了source_location，标准为20
set(CMAKE_CXX_STANDARD 20)
# 需要cpp文件形成库
set(LIB_SRC
    sylar/log.cpp
    sylar/util.cpp
    sylar/config.cpp
    sylar/thread.cpp
    )

# boost和yaml-cpp用的dnf安装，且为默认位置，无需配置include路径
# boost::lexical_cast 是 header-only，不需要链接boost的库-lboost_*
# yaml-cpp则非header-only，需要链接库
find_package(yaml-cpp REQUIRED)

add_library(sylar SHARED ${LIB_SRC})
# syalr本身就需要链接yaml-cpp，链接之后测试文件就不需要额外链接yaml-cpp了
# 公共依赖，完全传递
target_link_libraries(sylar PUBLIC yaml-cpp::yaml-cpp)

# add_executable(test tests/test.cpp)
# # target_link_libraries 本身就会建立构建顺序依赖
# # add_dependencies(test sylar)
# target_link_libraries(test sylar)

add_executable(test_config tests/test_config.cpp)
target_link_libraries(test_config sylar)

add_executable(test_thread tests/test_thread.cpp)
target_link_libraries(test_thread sylar)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
