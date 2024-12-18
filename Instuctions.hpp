#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
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
using namespace std;
class Instructions {
public:
  void processOpcodes() {
    int offset = 0;
    int total_process_var_count = 0;
    for (int i = 0; i < local_var_declare_count; ++i) {
      const unsigned int var_count_in_this_declare = stoul(instr_vec[offset], nullptr, 16);
      const string var_type_in_this_declare = instr_vec[offset + 1];
      for (int j = 0; j < var_count_in_this_declare; ++j) {
        if (var_type_in_this_declare == "7f") {
          local_int32.push_back(0);
          local_var_indexer[total_process_var_count] = local_int32.size() - 1;
          local_var_type_getter[total_process_var_count] = "i32";
        } else if (var_type_in_this_declare == "7e") {
          local_int64.push_back(0);
          local_var_indexer[total_process_var_count] = local_int64.size() - 1;
          local_var_type_getter[total_process_var_count] = "i64";
        } else if (var_type_in_this_declare == "7d") {
          local_f32.push_back(0);
          local_var_indexer[total_process_var_count] = local_f32.size() - 1;
          local_var_type_getter[total_process_var_count] = "f32";
        } else if (var_type_in_this_declare == "7c") {
          local_f64.push_back(0);
          local_var_indexer[total_process_var_count] = local_f64.size() - 1;
          local_var_type_getter[total_process_var_count] = "f64";
        }
        total_process_var_count += 1;
      }
      offset += 2;
    }
    cout << local_int32 << local_int64 << local_f32 << local_f64;
    cout << local_var_indexer << local_var_type_getter;
    for (int i = offset; i < instr_vec.size();++i){
      cout << instr_vec[i] << " ";
    }
    // while (i < instr_vec.size()) {
    //   if (instr_vec[i] == "0f") { // ret
    //     emitRet();
    //   } else if (instr_vec[i] == "0b") { // end
    //     emitRet();
    //   }
    //   ++i;
    // }
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
  vector<int32_t> local_int32;
  vector<int64_t> local_int64;
  vector<float> local_f32;
  vector<double> local_f64;
  map<int, int> local_var_indexer;
  map<int, string> local_var_type_getter;
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
  unordered_map<std::string, std::string> number_type_table = {{"7F", "int"}, {"7E", "long"}, {"7D", "float"}, {"7C", "double"}};
};