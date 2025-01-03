#include <iostream>
#include <variant>
#include <stdexcept>
#include <type_traits>

// 定义 wasm_type
using wasm_type = std::variant<int32_t, int64_t, float, double>;

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

int main() {
    wasm_type v1 = int32_t(10);
    wasm_type v2 = int64_t(20);
    wasm_type v3 = double(10.5);
    wasm_type v4 = float(5.5);

    try {
        wasm_type result1 = v1 + v2;
        std::visit([](auto&& arg) { std::cout << "Result1: " << arg << std::endl; }, result1);

        wasm_type result2 = v3 + v4;
        std::visit([](auto&& arg) { std::cout << "Result2: " << arg << std::endl; }, result2);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}