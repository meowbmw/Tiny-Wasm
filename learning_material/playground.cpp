#include <iostream>
#include <variant>

// 定义 wasm_type
using wasm_type = std::variant<int32_t, int64_t, float, double>;

// 重载 << 运算符
std::ostream& operator<<(std::ostream& os, const wasm_type& value) {
    std::visit([&os](auto&& arg) {
        os << arg;
    }, value);
    return os;
}

int main() {
    wasm_type v1 = int32_t(10);
    wasm_type v2 = int64_t(20);
    wasm_type v3 = float(10.5);
    wasm_type v4 = double(5.5);

    std::cout << "v1: " << v1 << std::endl;
    std::cout << "v2: " << v2 << std::endl;
    std::cout << "v3: " << v3 << std::endl;
    std::cout << "v4: " << v4 << std::endl;

    return 0;
}