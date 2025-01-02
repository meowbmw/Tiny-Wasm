#include "include/WasmFunction.hpp"
#include "include/WasmType.hpp"
#include "include/json.hpp"

using namespace std;
using json = nlohmann::json;
// aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
// aarch64-linux-gnu-g++ parser.cpp -o parser && qemu-aarch64 -L /usr/aarch64-linux-gnu ./parser
// qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu ./parser
const bool DEBUG_EXPORT_SECTION = false;
const bool DEBUG_FUNCTION_SECTION = false;
const bool DEBUG_TYPE_SECTION = false;
const bool DEBUG_CODE_SECTION = false;

// const bool DEBUG_EXPORT_SECTION = true;
// const bool DEBUG_FUNCTION_SECTION = true;
// const bool DEBUG_TYPE_SECTION = true;
// const bool DEBUG_CODE_SECTION = true;
const string WASM_TO_READ = "test/local.2.wasm";
void initial_check(string &s) {
  // check magic number and version
  cout << "Full Binary: " << s << endl;
  const string magic_number = s.substr(0, 8);
  cout << "Initial checking..\nMagic number: " << ((magic_number == "0061736d") ? "Matched" : "Unmatched") << endl;
  s = s.substr(8); // crop magic number
  cout << "Webassembly version is: " << s.substr(0, 2) << endl;
  s = s.substr(8); // crop version
}
class Parser {
public:
  void set(const std::string &str, const int &num) {
    s = str;
    length = num;
  }
  void parse_type() {
    // type section
    const u_int64_t type_count = stoul(s.substr(0, 2), nullptr, 16);
    uint64_t base_offset = 2; // Warn: skip one byte: 60 (function type identifier) as it is fixed
    cout << "Decoding type section: " << s.substr(0, length * 2) << endl;
    cout << "Total type count: " << type_count << endl;
    for (int i = 0; i < type_count; ++i) {
      WasmType curType;
      const uint64_t param_count = stoul(s.substr(base_offset + 2, 2), nullptr, 16);
      base_offset = base_offset + 2;
      for (int j = 0; j < param_count; ++j) {
        curType.add_param(s.substr(base_offset + 2 + 2 * j, 2));
      }
      base_offset = base_offset + 2 + 2 * param_count;
      const uint64_t result_count = stoul(s.substr(base_offset, 2), nullptr, 16);
      for (int j = 0; j < result_count; ++j) {
        curType.add_result(s.substr(base_offset + 2 + 2 * j, 2));
      }
      if (DEBUG_TYPE_SECTION) {
        cout << "--- Info for type " << i << " ---" << endl;
        curType.print_data(TypeCategory::PARAM);
        curType.print_data(TypeCategory::RESULT);
      }
      base_offset = base_offset + 2 + 2 * result_count;
      wasmTypeVec.push_back(curType);
    }
  }
  void parse_function() {
    // function section
    const u_int64_t function_count = stoul(s.substr(0, 2), nullptr, 16);
    uint64_t base_offset = 0;
    cout << "Decoding function section: " << s.substr(0, length * 2) << endl;
    cout << "Total function count: " << function_count << endl;
    for (int i = 0; i < function_count; ++i) {
      auto function_type = stoul(s.substr(base_offset + 2 * i + 2, 2), nullptr, 16);
      funcTypeVec.push_back(function_type);
      if (DEBUG_FUNCTION_SECTION) {
        cout << "--- Info for function " << i << " ---" << endl;
        cout << "Type is: " << function_type << endl;
      }
    }
  }
  void parse_export() {
    // export section
    const u_int64_t export_count = stoul(s.substr(0, 2), nullptr, 16);
    uint64_t base_offset = 0;
    cout << "Decoding export section: " << s.substr(0, length * 2) << endl;
    cout << "Total export count: " << export_count << endl;
    for (int i = 0; i < export_count; ++i) {
      const u_int64_t export_size = stoul(s.substr(base_offset + 2, 2), nullptr, 16);
      const string export_name = hexToAscii(s.substr(base_offset + 4, export_size * 2));
      const u_int64_t export_type = stoul(s.substr(base_offset + 2 * export_size + 4, 2), nullptr, 16);
      const u_int64_t export_index = stoul(s.substr(base_offset + 2 * export_size + 6, 2), nullptr, 16);
      funcNameIndexMapper[export_name] = export_index;
      funcIndexNameMapper[export_index] = export_name;
      if (DEBUG_EXPORT_SECTION) {
        cout << "--- Info for export " << i << " ---" << endl;
        cout << "Type: " << export_type << endl;
        cout << "Index: " << export_index << endl;
        cout << "Name: " << export_name << endl;
      }
      base_offset = base_offset + 2 * export_size + 6;
    }
  }
  void parse_code() {
    // code section
    const u_int64_t func_count = stoul(s.substr(0, 2), nullptr, 16);
    uint64_t base_offset = 0;
    cout << "Decoding code section: " << s.substr(0, length * 2) << endl;
    cout << "Total function count: " << func_count << endl;
    for (int i = 0; i < func_count; ++i) {
      WasmFunction curFunc;
      const u_int64_t func_size = stoul(s.substr(base_offset + 2, 2), nullptr, 16);
      const u_int64_t local_var_declare_count =
          stoul(s.substr(base_offset + 4, 2), nullptr,
                16); // Warn!!! One declare could imply multiple variables so this does not really equal to the real variable count!!!
      if (DEBUG_CODE_SECTION) {
        cout << "--- Info for function " << i << " ---" << endl;
        cout << "Func size: " << func_size << endl;
        cout << "Local variable declare count: " << local_var_declare_count << endl;
      }
      vector<string> opcodes;
      for (int i = 0; i < func_size - 1; ++i) {
        opcodes.push_back(s.substr(base_offset + 6 + i * 2, 2));
      }
      if (DEBUG_CODE_SECTION) {
        cout << "Opcode: ";
        for (auto &c : opcodes) {
          cout << c << " ";
        }
        cout << endl;
      }
      base_offset = base_offset + 2 * func_size + 2;
      curFunc.set_code_vec(opcodes, local_var_declare_count);
      wasmFunctionVec.push_back(curFunc);
    }
  }
  void funcBatchProcess(bool execute = false) {
    // give function their respective param, result and local vec.
    // generate respective machine code
    for (int i = 0; i < funcTypeVec.size(); ++i) {
      wasmFunctionVec[i].type = funcTypeVec[i];
      wasmFunctionVec[i].param_data = wasmTypeVec[funcTypeVec[i]].param_data;
      wasmFunctionVec[i].result_data = wasmTypeVec[funcTypeVec[i]].result_data;
      cout << "------ Processing function " << i << ": " << funcIndexNameMapper[i] << " ------" << endl;
      wasmFunctionVec[i].prepareParams();
      wasmFunctionVec[i].processCodeVec(); // this function will deal with local varaible initialization and machine code construction
      cout << "Total param count: " << wasmFunctionVec[i].param_data.size() << endl;
      cout << "Total local count: " << wasmFunctionVec[i].local_data.size()
           << endl; // NOTE: only output local count after processCodeVec or it will be wrong number!
      cout << "Total result count: " << wasmFunctionVec[i].result_data.size() << endl;
      if (execute) {
        cout << "Executing function " << i << ": " << funcIndexNameMapper[i] << endl;
        wasmFunctionVec[i].executeInstr();
      }
    }
  }
  string s;                             // should be read-only
  unsigned int length = 0;              // should be read-only
  vector<WasmFunction> wasmFunctionVec; // used to store function code
  vector<WasmType> wasmTypeVec;         // used to store type definition
  vector<int> funcTypeVec;
  map<string, int> funcNameIndexMapper;
  map<int, string> funcIndexNameMapper;
};

int main() {
  /***
   * TODO:
   * （1） support multi param function pointer -> save to x0 (which is a pointer)
   * also need to support int32/int64/void return
   *  (2) move c++ vector to assembly; maybe save it to sp+offset?
   * NEED TO EMIT MACHINE CODE FOR EVERY WASM LINE!
   *
   */
  cout << "Parsing wasm file: " << WASM_TO_READ << endl;
  string s = readBinary(WASM_TO_READ);
  initial_check(s);
  Parser parser;
  while (s.size() > 0) {
    const string type = s.substr(0, 2);
    s = s.substr(2);                                                // crop type
    const unsigned int length = stoul(s.substr(0, 2), nullptr, 16); // Warn & TODO: we assume that the length is at max 1 byte!!!
    s = s.substr(2);                                                // crop length
    parser.set(s, length);
    if (type == "01") {
      parser.parse_type();
    }
    if (type == "03") {
      parser.parse_function();
    }
    if (type == "07") {
      parser.parse_export();
    }
    if (type == "0a") {
      parser.parse_code();
    }
    s = s.substr(length * 2); // move forward, remeber we need to times 2 because we are processing 2 char at a time; 2 char = 2 * 4 bits = 1 byte
    // cout << type << " " << length << endl;
  }
  parser.funcBatchProcess(true);
}