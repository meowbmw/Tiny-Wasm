#pragma once
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include "Utils.h"
// 重载 + 运算符
wasm_type operator+(const wasm_type& a, const wasm_type& b) {
    return std::visit([](auto&& arg1, auto&& arg2) -> wasm_type {
        using T1 = std::decay_t<decltype(arg1)>;
        using T2 = std::decay_t<decltype(arg2)>;

        if constexpr (std::is_same_v<T1, T2>) {
            return arg1 + arg2;
        } else if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
            return static_cast<std::common_type_t<T1, T2>>(arg1) + static_cast<std::common_type_t<T1, T2>>(arg2);
        } else {
            throw std::invalid_argument("Unsupported types in operator+");
        }
    }, a, b);
}
wasm_type operator-(const wasm_type& a, const wasm_type& b) {
    return std::visit([](auto&& arg1, auto&& arg2) -> wasm_type {
        using T1 = std::decay_t<decltype(arg1)>;
        using T2 = std::decay_t<decltype(arg2)>;

        if constexpr (std::is_same_v<T1, T2>) {
            return arg1 - arg2;
        } else if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
            return static_cast<std::common_type_t<T1, T2>>(arg1) - static_cast<std::common_type_t<T1, T2>>(arg2);
        } else {
            throw std::invalid_argument("Unsupported types in operator-");
        }
    }, a, b);
}
wasm_type operator*(const wasm_type& a, const wasm_type& b) {
    return std::visit([](auto&& arg1, auto&& arg2) -> wasm_type {
        using T1 = std::decay_t<decltype(arg1)>;
        using T2 = std::decay_t<decltype(arg2)>;

        if constexpr (std::is_same_v<T1, T2>) {
            return arg1 * arg2;
        } else if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
            return static_cast<std::common_type_t<T1, T2>>(arg1) * static_cast<std::common_type_t<T1, T2>>(arg2);
        } else {
            throw std::invalid_argument("Unsupported types in operator*");
        }
    }, a, b);
}
wasm_type operator/(const wasm_type& a, const wasm_type& b) {
    return std::visit([](auto&& arg1, auto&& arg2) -> wasm_type {
        using T1 = std::decay_t<decltype(arg1)>;
        using T2 = std::decay_t<decltype(arg2)>;

        if constexpr (std::is_same_v<T1, T2>) {
            return arg1 / arg2;
        } else if constexpr (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>) {
            return static_cast<std::common_type_t<T1, T2>>(arg1) / static_cast<std::common_type_t<T1, T2>>(arg2);
        } else {
            throw std::invalid_argument("Unsupported types in operator/");
        }
    }, a, b);
}
// 重载 wasm_type << 运算符
std::ostream& operator<<(std::ostream& os, const wasm_type& value) {
    std::visit([&os](auto&& arg) {
        os << arg;
    }, value);
    return os;
}
// 模板函数重载 << 操作符以支持任意类型的 std::vector 打印
template <typename T> std::ostream &operator<<(std::ostream &os, const std::vector<T> &vec) {
  os << "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    std::visit(
        [&os](auto &&value) {
          os << value;
        },
        vec[i]);
    if (i != vec.size() - 1) {
      os << ", ";
    }
  }
  os << "]";
  return os;
}

// 模板函数重载 << 操作符以支持任意类型的 std::map 打印
template <typename K, typename V> std::ostream &operator<<(std::ostream &os, const std::map<K, V> &m) {
  os << "{";
  auto it = m.begin();
  while (it != m.end()) {
    os << it->first << ": " << it->second;
    if (++it != m.end()) {
      os << ", ";
    }
  }
  os << "}";
  return os;
}

// 模板函数重载 << 操作符以支持任意类型的 std::unordered_map 打印
template <typename K, typename V> std::ostream &operator<<(std::ostream &os, const std::unordered_map<K, V> &um) {
  os << "{";
  auto it = um.begin();
  while (it != um.end()) {
    os << it->first << ": " << it->second;
    if (++it != um.end()) {
      os << ", ";
    }
  }
  os << "}";
  return os;
}