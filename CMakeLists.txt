cmake_minimum_required(VERSION 3.0)
project(sylar-source-study)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS}-rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

set(CMAKE_CXX_STANDARD 20)          # 设置 C++ 标准为 C++20
set(LIB_SRC
    sylar/log.cpp
    sylar/util.cpp
    sylar/config.cpp
    )

add_library(sylar SHARED ${LIB_SRC})


add_executable(test tests/test.cpp)
# add_dependencies(test sylar)
# target_link_libraries 本身就会建立构建顺序依赖
target_link_libraries(test sylar)

add_executable(test_config tests/test_config.cpp)
target_link_libraries(test_config sylar)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
