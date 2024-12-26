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
#include <format>
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
