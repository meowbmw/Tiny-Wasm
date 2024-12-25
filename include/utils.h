#pragma once
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>
using namespace std;
using wasm_type = std::variant<int32_t, int64_t, float, double>;
enum class TypeCategory { PARAM, RESULT, LOCAL };
string type_category_to_string(TypeCategory category) {
  switch (category) {
  case TypeCategory::PARAM:
    return "PARAM";
  case TypeCategory::RESULT:
    return "RESULT";
  case TypeCategory::LOCAL:
    return "LOCAL";
  default:
    return "UNKNOWN";
  }
}
void erase_space(string &s) {
  s.erase(remove_if(s.begin(), s.end(),
                    [](unsigned char x) {
                      return std::isspace(x);
                    }),
          s.end());
}
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

string HexToBinary(string s) {
  string result;
  for (auto &c : s) {
    switch (c) {
    case '0':
      result += "0000";
      continue;
    case '1':
      result += "0001";
      continue;
    case '2':
      result += "0010";
      continue;
    case '3':
      result += "0011";
      continue;
    case '4':
      result += "0100";
      continue;
    case '5':
      result += "0101";
      continue;
    case '6':
      result += "0110";
      continue;
    case '7':
      result += "0111";
      continue;
    case '8':
      result += "1000";
      continue;
    case '9':
      result += "1001";
      continue;
    case 'a':
      result += "1010";
      continue;
    case 'b':
      result += "1011";
      continue;
    case 'c':
      result += "1100";
      continue;
    case 'd':
      result += "1101";
      continue;
    case 'e':
      result += "1110";
      continue;
    case 'f':
      result += "1111";
      continue;
    case 'A':
      result += "1010";
      continue;
    case 'B':
      result += "1011";
      continue;
    case 'C':
      result += "1100";
      continue;
    case 'D':
      result += "1101";
      continue;
    case 'E':
      result += "1110";
      continue;
    case 'F':
      result += "1111";
      continue;
    default:
      return "bad input";
    }
  }
  return result;
}

// 将32位浮点数转换为其二进制表示
uint32_t float_to_bits(float value) {
  uint32_t result;
  std::memcpy(&result, &value, sizeof(value));
  return result;
}

// 将 uint32_t 类型的二进制表示转换回 float
float bits_to_float(uint32_t bits) {
    float result;
    std::memcpy(&result, &bits, sizeof(bits));
    return result;
}

// 将64位浮点数转换为其二进制表示
uint64_t double_to_bits(double value) {
  uint64_t result;
  std::memcpy(&result, &value, sizeof(value));
  return result;
}

// 将 uint32_t 类型的二进制表示转换回 float
double bits_to_float(uint64_t bits) {
    double result;
    std::memcpy(&result, &bits, sizeof(bits));
    return result;
}

string processHexCode(string &s, bool endian = false) {
  /**
   * Big endian -> true
   * Small endian -> false
   */
  erase_space(s);
  const size_t arraySize = s.length() / 2;
  string result;
  for (size_t i = 0; i < arraySize; i++) {
    if (endian == false) {
      // 转换回去的时候注意要逆序读
      result += HexToBinary(s.substr(arraySize * 2 - i * 2 - 2, 2));
    } else {
      result += HexToBinary(s.substr(i * 2, 2));
    }
  }
  return result;
}

// Helper function to extract bits from a binary string
unsigned int GetBits(const std::string &bits, int start, int end) {
  assert(start >= 0 && end < bits.size() && start <= end);
  std::string bitRange = bits.substr(bits.size() - end - 1, end - start + 1);
  return std::bitset<32>(bitRange).to_ulong();
}

std::string toBinaryString(uint32_t value) {
  std::ostringstream oss;
  for (int i = 31; i >= 0; i--) {
    oss << ((value >> i) & 1);
    if (i % 4 == 0 && i != 0) {
      // 每4位加个空格，只是为了看得更清晰
      oss << " ";
    }
  }
  return oss.str();
}

// 将二进制字符串格式化为每 4 位一个空格
std::string formatBinaryString(const std::string &binaryString) {
    std::stringstream formatted;
    for (size_t i = 0; i < binaryString.size(); ++i) {
        formatted << binaryString[i];
        if ((i + 1) % 4 == 0 && (i + 1) != binaryString.size()) {
            formatted << ' ';
        }
    }
    return formatted.str();
}

std::string toHexString(uint32_t value) {
  std::ostringstream oss;
  oss << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
  return oss.str();
}
double hexToDouble(const std::string &hexStr) {
  uint64_t intVal;
  std::stringstream ss;
  ss << std::hex << hexStr;
  ss >> intVal;
  double doubleVal;
  std::memcpy(&doubleVal, &intVal, sizeof(doubleVal));
  return doubleVal;
}

float hexToFloat(const std::string &hexStr) {
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