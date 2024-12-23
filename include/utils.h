#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <string>
#include <cstring>
#include <map>
#include <memory>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <sys/mman.h>
#include <cstdint>
#include <variant>
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