#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <vector>

#include "json.hpp"

using namespace std;
using json = nlohmann::json;
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
class Section {
public:
  int length;
  int type;
  string descriptor;
};
class Instructions {
public:
  static void processOpcodes(vector<string> &vec) {
    for (auto &c : vec) {
      if (c == "0f") { // ret
        emitRet();
      } else if (c == "0b") { // end
        emitRet();
      }
    }
  }
  static void emitRet() {
    // cout << "Emitting Return" << endl;
    const string instr = "d65f03c0";
    excuteInst(instr);
    // cout << "Executed" << endl;
  }
  static void printHexArray(unsigned char *charArray, int arraySize) {
    for (size_t i = 0; i < arraySize; i++) {
      std::cout << std::hex << setw(2) << setfill('0') << static_cast<unsigned int>(charArray[i]) << " ";
    }
    cout << endl;
  }
  static void excuteInst(string s) {
    const size_t arraySize = s.length() / 2;
    unsigned char *const charArray = static_cast<unsigned char *>(malloc(arraySize));
    for (size_t i = 0; i < arraySize; i++) {
      const string byteStr =
          s.substr(arraySize * 2 - i * 2 - 2, 2); // little endian on arm so we need to do this reversely!! no need to do this on x86
      charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    void (*func)() = nullptr;
    // allocate executable buffer
    func = reinterpret_cast<void (*)()>(mmap(nullptr, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (func == MAP_FAILED) {
      perror("mmap");
      free(charArray);
      return;
    }
    // copy code to buffer
    memcpy(reinterpret_cast<void *>(func), charArray, arraySize);
    // ensure memcpy isn't optimized away as a dead store.
    __builtin___clear_cache(reinterpret_cast<char *>(func), reinterpret_cast<char *>(func) + arraySize);
    func();
    munmap(reinterpret_cast<void *>(func), arraySize);
    free(charArray);
  }
};
int main() {
  // aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
  // aarch64-linux-gnu-g++ parser.cpp -o parser && qemu-aarch64 -L /usr/aarch64-linux-gnu ./parser
  string wasm_to_read = "test/local.0.wasm";
  string s = readBinary(wasm_to_read);
  cout << "Full Binary: " << s << endl;
  const string magic_number = s.substr(0, 8);
  cout << "Initial checking..\nMagic number: " << ((magic_number == "0061736d") ? "Matched" : "Unmatched") << endl;
  s = s.substr(8); // crop magic number
  cout << "Webassembly version is: " << s.substr(0, 2) << endl;
  s = s.substr(8); // crop version
  while (s.size() > 0) {
    const string type = s.substr(0, 2);
    s = s.substr(2);                                                // crop type
    const unsigned int length = stoul(s.substr(0, 2), nullptr, 16); // Warn: we assume that the length is at max 1 byte!!!
    s = s.substr(2);                                                // crop length
    if (type == "07") {
      // export section
      cout << "Decoding export section: " << s.substr(0, length * 2) << endl;
      const u_int64_t export_count = stoul(s.substr(0, 2), nullptr, 16);
      cout << "Total export count: " << export_count << endl;
      uint64_t base_offset = 0;
      for (int i = 0; i < export_count; ++i) {
        const u_int64_t export_size = stoul(s.substr(base_offset + 2, 2), nullptr, 16);
        cout << "--- Info for export " << i << " ---" << endl;
        const string export_name = hexToAscii(s.substr(base_offset + 4, export_size * 2));
        const u_int64_t export_type = stoul(s.substr(base_offset + 2 * export_size + 4, 2), nullptr, 16);
        const u_int64_t export_index = stoul(s.substr(base_offset + 2 * export_size + 6, 2), nullptr, 16);
        cout << "Type: " << export_type << endl;
        cout << "Index: " << export_index << endl;
        cout << "Name: " << export_name << endl;
        base_offset = base_offset + 2 * export_size + 6;
      }
    }
    if (type == "0a") {
      // code section
      cout << "Decoding code section: " << s.substr(0, length * 2) << endl;
      const u_int64_t func_count = stoul(s.substr(0, 2), nullptr, 16);
      cout << "Total function count: " << func_count << endl;
      uint64_t base_offset = 0;
      for (int i = 0; i < func_count; ++i) {
        const u_int64_t func_size = stoul(s.substr(base_offset + 2, 2), nullptr, 16);
        const u_int64_t local_var_count = stoul(s.substr(base_offset + 4, 2), nullptr, 16);
        cout << "--- Info for function " << i << " ---" << endl;
        cout << "Func size: " << func_size << endl;
        cout << "Local var count: " << local_var_count << endl;
        vector<string> opcodes;
        for (int i = 0; i < func_size - 1; ++i) {
          opcodes.push_back(s.substr(base_offset + 6 + i * 2, 2));
        }
        cout << "Opcode: ";
        for (auto &c : opcodes) {
          cout << c << " ";
        }
        cout << endl;
        base_offset = base_offset + 2 * func_size + 2;
        //   Instructions::processOpcodes(opcodes);
      }
    }
    s = s.substr(length * 2); // move forward, remeber we need to times 2 because we are processing 2 char at a time; 2 char = 1 byte
    // cout << type << " " << length << endl;
  }
}