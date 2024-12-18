cmake_minimum_required(VERSION 3.14)

# 设置编译器
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# 设置生成目标架构
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

set(CMAKE_CXX_CLANG_TIDY clang-tidy --config-file ${CMAKE_CURRENT_LIST_DIR}/.clang-tidy)

project(ARM64)


# 添加测试
add_executable(parser parser.cpp)

target_link_libraries(parser)


# cmake -S . -B build && cmake --build build
# cmake -S . -B build && cmake --build build && cd ./build && ctest && cd .. 