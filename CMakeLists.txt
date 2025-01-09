cmake_minimum_required(VERSION 3.14)
# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

# 设置编译器
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set (CMAKE_CXX_STANDARD 20)
# 设置生成目标架构
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address") # DISABLED because this is causing segmentation fault: https://github.com/google/sanitizers/issues/1614

# set(CMAKE_CXX_CLANG_TIDY clang-tidy --config-file ${CMAKE_CURRENT_LIST_DIR}/.clang-tidy) # DISABLED because clang-tidy is reporting too much warning in gtest code

project(WASM_COMPILER_LEARN)

# Avoid complain about DartConfiguration.tcl not found
include(CTest)
# 添加GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_language(ASM)

# 添加测试
add_executable(wasm_test wasmtest.cc)
enable_testing()

target_link_libraries(wasm_test gtest_main)

# 定义测试编号数组
set(TEST_NUMBERS 2 3 4)

# 遍历测试编号数组，自动生成测试
foreach(TEST_NUM IN LISTS TEST_NUMBERS)
    add_test(
        NAME wasm_test${TEST_NUM}
        COMMAND "qemu-aarch64" -L /usr/aarch64-linux-gnu wasm_test ${TEST_NUM}
    )
endforeach()
# rm -rf build && cmake -S . -B build && cmake --build build
# rm -rf build && cmake -S . -B build && cmake --build build && cd ./build && ctest; cd .. 