#include <iostream>
#include <functional>
#include <stdarg.h>

// 定义一个辅助函数来调用通用函数指针
template<typename Func, typename... Args>
void callFunctionHelper(Func func, Args... args) {
    func(std::forward<Args>(args)...);
}

// 示例函数1：无参数
void exampleFunction1() {
    std::cout << "exampleFunction1 called!" << std::endl;
}

// 示例函数2：带参数
void exampleFunction2(int arg) {
    std::cout << "exampleFunction2 called with arg: " << arg << std::endl;
}

// 示例函数3：可变参数
void exampleFunction3(int count, ...) {
    std::cout << "exampleFunction3 called with " << count << " args: ";
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        int arg = va_arg(args, int);
        std::cout << arg << " ";
    }
    va_end(args);
    std::cout << std::endl;
}

int main() {
    // 调用 exampleFunction1
    callFunctionHelper(exampleFunction1);

    // 调用 exampleFunction2
    callFunctionHelper(exampleFunction2, 42);

    // 调用 exampleFunction3，使用可变参数
    // 注意：为了简化，直接传递参数而不是使用变长参数列表
    callFunctionHelper(exampleFunction3, 3, 10, 20, 30);

    return 0;
}