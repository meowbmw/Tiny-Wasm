#pragma once
#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>

// 模板函数重载 << 操作符以支持任意类型的 std::vector 打印
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        os << vec[i];
        if (i != vec.size() - 1) {
            os << ", ";
        }
    }
    os << "]" << std::endl;
    return os;
}

// 模板函数重载 << 操作符以支持任意类型的 std::map 打印
template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m) {
    os << "{";
    auto it = m.begin();
    while (it != m.end()) {
        os << it->first << ": " << it->second;
        if (++it != m.end()) {
            os << ", ";
        }
    }
    os << "}" << std::endl;
    return os;
}

// 模板函数重载 << 操作符以支持任意类型的 std::unordered_map 打印
template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, V>& um) {
    os << "{";
    auto it = um.begin();
    while (it != um.end()) {
        os << it->first << ": " << it->second;
        if (++it != um.end()) {
            os << ", ";
        }
    }
    os << "}" << std::endl;
    return os;
}