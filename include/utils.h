#include <iostream>
#include <variant>
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <string>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <sys/mman.h>
#include <cstdint>
using namespace std;

class VariantArray {
public:
    // 定义一个可以存储 int, double 和 std::string 的 variant
    using VarType = std::variant<int32_t, int64_t, float, double>;

    // 函数模板，用于将不同类型的变量 push 到数组中
    template <typename T>
    void push(T value) {
        varArray.push_back(value);
    }

    // 函数模板，用于通过索引访问 VarType 中存储的值
    template <typename T>
    T get(size_t index) const {
        if (index >= varArray.size()) {
            throw std::out_of_range("Index out of range");
        }
        return std::get<T>(varArray[index]);
    }

    // 函数模板，用于通过索引访问 VarType 中存储的值并实例化一个对应的变量
    void instantiateVariable(size_t index) const {
        if (index >= varArray.size()) {
            throw std::out_of_range("Index out of range");
        }

        // 使用 std::visit 访问 variant 中的值并实例化一个对应的变量
        std::visit([](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            T variable = value; // 实例化一个对应类型的变量
            std::cout << "Instantiated variable of type " << typeid(T).name() << " with value: " << variable << std::endl;
        }, varArray[index]);
    }

    // 函数用于打印 VarType 中存储的值
    void print() const {
        for (const auto& var : varArray) {
            std::visit([](const auto& value) {
                std::cout << value << std::endl;
            }, var);
        }
    }

private:
    // 定义一个 vector 来存储 VarType
    std::vector<VarType> varArray;
};
// Helper function to convert a single hex character to its integer value
int hexCharToInt(char ch) {
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  throw std::invalid_argument("Invalid hex character");
}

// Function to convert hex string to ASCII string
string hexToAscii(const std::string &hex) {
  if (hex.length() % 2 != 0) {
    throw invalid_argument("Hex string length must be even");
  }

  string ascii;
  ascii.reserve(hex.length() / 2);

  for (size_t i = 0; i < hex.length(); i += 2) {
    char high = hexCharToInt(hex[i]);
    char low = hexCharToInt(hex[i + 1]);
    ascii.push_back((high << 4) | low);
  }

  return ascii;
}

double hexToDouble(const std::string& hexStr) {
    uint64_t intVal;
    std::stringstream ss;
    ss << std::hex << hexStr;
    ss >> intVal;
    double doubleVal;
    std::memcpy(&doubleVal, &intVal, sizeof(doubleVal));
    return doubleVal;
}


float hexToFloat(const std::string& hexStr) {
    uint32_t intVal;
    std::stringstream ss;
    ss << std::hex << hexStr;
    ss >> intVal;
    float floatVal;
    std::memcpy(&floatVal, &intVal, sizeof(floatVal));
    return floatVal;
}

string floatToHex(float floatVal) {
    uint32_t intVal;
    std::memcpy(&intVal, &floatVal, sizeof(floatVal));

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << intVal;
    return ss.str();
}

string doubleToHex(double doubleVal) {
    uint64_t intVal;
    std::memcpy(&intVal, &doubleVal, sizeof(doubleVal));

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << intVal;
    return ss.str();
}

string readBinary(string wasm_source) {
  ifstream file(wasm_source, ios::binary);
  stringstream ss;
  char byte;
  while (file.get(byte)) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(byte));
  }
  file.close();
  return ss.str();
}

void printHexArray(unsigned char *charArray, int arraySize) {
    for (size_t i = 0; i < arraySize; i++) {
      std::cout << std::hex << setw(2) << setfill('0') << static_cast<unsigned int>(charArray[i]) << " ";
    }
    cout << endl;
}