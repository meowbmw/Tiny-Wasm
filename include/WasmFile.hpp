#pragma once
#include "Wasm.hpp"
#include "Wasm/WasmFunction.hpp"
#include "Wasm/WasmFunctionType.hpp"
using namespace std;

const bool DEBUG_EXPORT_SECTION = false;
const bool DEBUG_FUNCTION_SECTION = false;
const bool DEBUG_TYPE_SECTION = false;
const bool DEBUG_CODE_SECTION = false;

// const bool DEBUG_EXPORT_SECTION = true;
// const bool DEBUG_FUNCTION_SECTION = true;
// const bool DEBUG_TYPE_SECTION = true;
// const bool DEBUG_CODE_SECTION = true;
const string WASM_TO_READ = "test/local.2.wasm";

class WasmFile {
public:
  void parse() {
    cout << "Parsing wasm file: " << WASM_PATH << endl;
    initial_check();
    while (s.size() > 0) {
      const string type = s.substr(0, 2);
      s = s.substr(2); // crop type
      auto [value, bytesRead] = decode_uleb128(s, 0);
      length = value;
      s = s.substr(bytesRead);
      if (type == "01") {
        parse_type();
      } else if (type == "03") {
        parse_function();
      } else if (type == "07") {
        parse_export();
      } else if (type == "0a") {
        parse_code();
      }
      s = s.substr(length * 2); // move forward, remeber we need to times 2 because we are processing 2 char at a time; 2 char = 2 * 4 bits = 1 byte
      // cout << type << " " << length << endl;
    }
  }
  void initial_check() {
    // check magic number and version
    cout << "Full Binary: " << s << endl;
    const string magic_number = s.substr(0, 8);
    if (magic_number == "0061736d") {
      cout << "Initial checking..\nMagic number: Matched" << endl;
    } else {
      cout << "Initial checking..\nMagic number: Unmatched" << endl;
      cout << "Aborting" << endl;
      exit(-1);
    }
    s = s.substr(8); // crop magic number
    cout << "Webassembly version is: " << s.substr(0, 2) << endl;
    s = s.substr(8); // crop version
  }
  void parse_type() {
    // type section
    auto [type_count, bytes_read] = decode_uleb128(s, 0);
    uint64_t base_offset = 2; // Warn: skip one byte: 60 (function type identifier) as it is fixed
    cout << "Decoding type section: " << s.substr(0, length * 2) << endl;
    cout << "Total type count: " << type_count << endl;
    for (int i = 0; i < type_count; ++i) {
      WasmFunctionType curType;
      auto [param_count, bytesRead_param] = decode_uleb128(s, base_offset + bytes_read);
      base_offset = base_offset + bytesRead_param;
      for (int j = 0; j < param_count; ++j) {
        curType.add_data(TypeCategory::PARAM, s.substr(base_offset + bytesRead_param + 2 * j, 2));
      }
      base_offset = base_offset + 2 + 2 * param_count;
      auto [result_count, bytesRead_result] = decode_uleb128(s, base_offset);
      for (int j = 0; j < result_count; ++j) {
        curType.add_data(TypeCategory::RESULT, s.substr(base_offset + bytesRead_result + 2 * j, 2));
      }
      if (DEBUG_TYPE_SECTION) {
        cout << "--- Info for type " << i << " ---" << endl;
        curType.print_data(TypeCategory::PARAM);
        curType.print_data(TypeCategory::RESULT);
      }
      base_offset = base_offset + bytesRead_result + 2 * result_count;
      wasmFunctionTypeVec.push_back(curType);
    }
  }
  void parse_function() {
    // function section
    auto [function_count, bytes_read] = decode_uleb128(s, 0);
    uint64_t base_offset = bytes_read;
    cout << "Decoding function section: " << s.substr(0, length * 2) << endl;
    cout << "Total function count: " << function_count << endl;
    for (int i = 0; i < function_count; ++i) {
      auto [function_type, bytes_read_type] = decode_uleb128(s, base_offset);
      wasmFunctionToTypeMapper.push_back(function_type);
      if (DEBUG_FUNCTION_SECTION) {
        cout << "--- Info for function " << i << " ---" << endl;
        cout << "Type is: " << function_type << endl;
      }
      base_offset += bytes_read_type;
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
  // preallocate memory for function and save it to symbol table
  // it will be written when real code is generated
  void preAllocateMemory(int i) {
    size_t estimatedSize = wasmFunctionVec[i].code_vec.size() * 8; // a rough estimate of how much memory to allocate based on code_vec.size()
    estimatedSize = max(estimatedSize, size_t(4096));
    void *functionAddr = mmap(nullptr, estimatedSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (functionAddr == MAP_FAILED) {
      perror("mmap");
      exit(1);
    }
    symbol_table[i] = reinterpret_cast<void *>(functionAddr);
  }
  void writeCodeToMemory(int i) {
    // now that we have the jit code, we can fill in the blanks
    // TODO: refactor this into a function and add error handling
    const string instructions =
        wasmFunctionVec[i].prep_sp_instr + wasmFunctionVec[i].init_param_instr + wasmFunctionVec[i].init_local_instr +
        wasmFunctionVec[i].wasm_instructions.substr(wasmFunctionVec[i].jit_begin, wasmFunctionVec[i].jit_end - wasmFunctionVec[i].jit_begin) + wasmFunctionVec[i].restore_sp_instr + encodeReturn(30, true, false);
    char *functionAddr = reinterpret_cast<char *>(symbol_table[i]);
    const size_t arraySize = instructions.length() / 2;
    for (size_t j = 0; j < arraySize; j++) {
      const string byteStr = instructions.substr(j * 2, 2);
      functionAddr[j] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    __builtin___clear_cache(functionAddr, functionAddr + arraySize);
  }
  void initFunctionbyType(int i) {
    // assign name to wasmFunction
    if (funcIndexNameMapper.contains(i)) {
      wasmFunctionVec[i].functionName = funcIndexNameMapper[i];
    } else {
      // if its index is not present in export section, give it a predefined name
      wasmFunctionVec[i].functionName = format("unnamed_func_{}", i);
    }
    wasmFunctionVec[i].type = wasmFunctionToTypeMapper[i];
    wasmFunctionVec[i].param_data = wasmFunctionTypeVec[wasmFunctionToTypeMapper[i]].param_data;
    wasmFunctionVec[i].result_data = wasmFunctionTypeVec[wasmFunctionToTypeMapper[i]].result_data;
    wasmFunctionVec[i].wasmFunctionTypeVec = wasmFunctionTypeVec;
    wasmFunctionVec[i].wasmFunctionToTypeMapper = wasmFunctionToTypeMapper;
  }
  void funcSingleProcess(int i) {
    cout << "------ Processing function " << i << ": " << funcIndexNameMapper[i] << " ------" << endl;
    wasmFunctionVec[i].symbol_table = symbol_table;
    wasmFunctionVec[i].generatePreWasmInstructions();
    wasmFunctionVec[i].processCodeVec();
    writeCodeToMemory(i);
    // cout << "Total param count: " << wasmFunctionVec[i].param_data.size() << endl;
    // cout << "Total local count: " << wasmFunctionVec[i].local_data.size()
    //      << endl; // NOTE: only output local count after processCodeVec or it will be wrong number!
    // cout << "Total result count: " << wasmFunctionVec[i].result_data.size() << endl;
    cout << endl;
  }
  void funcBatchProcess(bool execute = false) {
    // give function their respective param, result and local vec.
    // generate respective machine code
    for (int i = 0; i < wasmFunctionToTypeMapper.size(); ++i) {
      initFunctionbyType(i);
      preAllocateMemory(i);
    }
    for (int i = 0; i < wasmFunctionToTypeMapper.size(); ++i) {
      funcSingleProcess(i);
      if (execute) {
        cout << "Executing function " << i << ": " << funcIndexNameMapper[i] << endl;
        wasmFunctionVec[i].executeWasmInstr();
      }
    }
  }
  WasmFile() {
  }
  WasmFile(string wasmpath) {
    WASM_PATH = wasmpath;
    s = readBinary(WASM_PATH);
  }

  string s;
  string WASM_PATH;
  int64_t result;
  unsigned int length = 0;
  vector<WasmFunction> wasmFunctionVec;         // used to store function code
  vector<WasmFunctionType> wasmFunctionTypeVec; // used to store type definition
  vector<int> wasmFunctionToTypeMapper;         // map function id to wasmType

  map<string, int> funcNameIndexMapper; // function name to index
  map<int, string> funcIndexNameMapper; // function index to name

  map<int, void *> symbol_table;
};