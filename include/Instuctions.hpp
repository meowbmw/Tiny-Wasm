#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>

#include "container_print.h"
#include "utils.h"
using namespace std;
using wasm_type = std::variant<int32_t, int64_t, float, double>;
class Instructions {
public:
  void processOpcodes() {
    int offset = 0;
    for (int i = 0; i < local_var_declare_count; ++i) {
      const unsigned int var_count_in_this_declare = stoul(instr_vec[offset], nullptr, 16);
      const string var_type_in_this_declare = instr_vec[offset + 1];
      for (int j = 0; j < var_count_in_this_declare; ++j) {
        if (var_type_in_this_declare == "7f") {
          local_data.push_back(static_cast<int32_t>(0));
        } else if (var_type_in_this_declare == "7e") {
          local_data.push_back(static_cast<int64_t>(0));
        } else if (var_type_in_this_declare == "7d") {
          local_data.push_back(static_cast<float>(0));
        } else if (var_type_in_this_declare == "7c") {
          local_data.push_back(static_cast<double>(0));
        }
      }
      offset += 2;
    }
    cout << "Executing instructions: ";
    for (int i = offset; i < instr_vec.size(); ++i) {
      cout << instr_vec[i] << " ";
    }
    cout << endl;
    int i = offset;
    while (i < instr_vec.size()) {
      if (instr_vec[i] == "0f") { // ret
        emitRet();
        ++i;
      } else if (instr_vec[i] == "0b") { // end
        emitRet();
        ++i;
      } else if (instr_vec[i] == "20") { // local.get
        const u_int64_t local_var_to_get = stoul(instr_vec[i + 1], nullptr, 16);
        if (local_var_to_get >= local_data.size()) {
          cout << "Too big index {" + to_string(local_var_to_get)  + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
        stack.push_back(local_data[local_var_to_get]);
        i += 2;
      } else if (instr_vec[i] == "21") { // local.set
        const u_int64_t local_var_to_set = stoul(instr_vec[i + 1], nullptr, 16);
        if (local_var_to_set >= local_data.size()) {
          cout << "Too big index {" + to_string(local_var_to_set) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
        local_data[local_var_to_set] = stack.back();
        stack.pop_back();
        i += 2;
      } else if (instr_vec[i] == "22") { // local.tee
        const u_int64_t local_var_to_set = stoul(instr_vec[i + 1], nullptr, 16);
        if (local_var_to_set >= local_data.size()) {
          cout << "Too big index {" + to_string(local_var_to_set) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
        local_data[local_var_to_set] = stack.back();
        // not pop back here since it's tee
        i += 2;
      } else if (instr_vec[i] == "41") { // i32.const
        stack.push_back(static_cast<int32_t>(stoul(instr_vec[i + 1], nullptr, 16)));
        i += 2;
      } else if (instr_vec[i] == "42") { // i64.const
        stack.push_back(static_cast<int64_t>(stoul(instr_vec[i + 1], nullptr, 16)));
        i += 2;
      } else if (instr_vec[i] == "43") { // f32.const
        stack.push_back(hexToFloat(instr_vec[i + 1] + instr_vec[i + 2] + instr_vec[i + 3] + instr_vec[i + 4]));
        i += 5;
      } else if (instr_vec[i] == "44") { // f64.const
        stack.push_back(hexToDouble(instr_vec[i + 1] + instr_vec[i + 2] + instr_vec[i + 3] + instr_vec[i + 4] + instr_vec[i + 5] + instr_vec[i + 6] +
                                    instr_vec[i + 7]));
        i += 9;
      }
    }
    print_local_data();
    print_stack();
  }
  void emitRet() {
    // cout << "Emitting Return" << endl;
    const string instr = "d65f03c0";
    loadInstr(instr);
    // cout << "Executed" << endl;
  }
  void loadInstr(string s) {
    /**
     * Allocate memory with execute permission
     * And load machine code into that
     * Save address pointer to self.functions
     */
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
    functions.push_back(func);
    functions_size.push_back(arraySize);
    free(charArray);
  }

  void print_local_data() {
    cout << "--- Printing local data---" << endl;
    for (auto &elem : local_data) {
      std::visit(
          [](auto &&value) {
            // value 的类型会被自动推断为四种之一
            std::cout << value << " is type: " << typeid(value).name() << std::endl;
          },
          elem);
    }
  }
  void print_stack() {
    cout << "--- Printing stack---" << endl;
    for (auto &elem : stack) {
      std::visit(
          [](auto &&value) {
            // value 的类型会被自动推断为四种之一
            std::cout << value << " is type: " << typeid(value).name() << std::endl;
          },
          elem);
    }
  }
  Instructions(vector<string> &v, size_t l = 0) {
    instr_vec = v;
    local_var_declare_count = l;
  }
  ~Instructions() {
    for (int i = 0; i < functions.size(); ++i) {
      munmap(reinterpret_cast<void *>(functions[i]), functions_size[i]);
    }
  }

private:
  u_int64_t func_count = 0;
  vector<string> instr_vec;
  u_int64_t local_var_declare_count = 0;
  vector<void (*)()> functions;
  vector<int> functions_size;
  vector<wasm_type> local_data;
  vector<wasm_type> stack;
  /**
   * we have 4 vectors
   * vector<int>, vector<double> ..
   * we have 5 variables, 1xint,2xdouble,1xlong,1xint
   * index is: 00,01,02,03,04
   * we need to be able to access by index
   * when adding to a vector, remeber its current index
   * like, index in locals 04, corresponding vector index 01
   * store this in an unordered_map
   */
};