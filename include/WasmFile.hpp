#pragma once
#include "Wasm.hpp"
#include "Wasm/WasmFunction.hpp"
#include "Wasm/WasmFunctionType.hpp"
using namespace std;

const bool DEBUG_EXPORT_SECTION = false;
const bool DEBUG_FUNCTION_SECTION = false;
const bool DEBUG_TYPE_SECTION = false;
const bool DEBUG_CODE_SECTION = false;
const bool DEBUG_TABLE_SECTION = false;
const bool DEBUG_ELEMENT_SECTION = false;
const bool DEBUG_GLOBAL_SECTION = true;

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
      } else if (type == "04") {
        parse_table();
      } else if (type == "06") {
        parse_global();
      } else if (type == "07") {
        parse_export();
      } else if (type == "09") {
        parse_element();
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
  void parse_table() {
    // table section
    auto [table_count, bytes_read] = decode_uleb128(s, 0);
    uint64_t base_offset = 2;
    cout << "Decoding table section: " << s.substr(0, length * 2) << endl;
    cout << "Total table count: " << table_count << endl;
    for (int i = 0; i < table_count; ++i) {
      const string elem_type = s.substr(base_offset, 2);
      base_offset += 2;

      if (DEBUG_TABLE_SECTION) {
        cout << "--- Info for table " << i << " ---" << endl;
        cout << "Element type: " << elem_type << " (should be 70 for funcref)" << endl;
      }
      const uint8_t limit_type = stoul(s.substr(base_offset, 2), nullptr, 16);
      base_offset += 2;
      auto [min_size, min_bytes_read] = decode_uleb128(s, base_offset);
      base_offset += min_bytes_read;
      uint64_t max_size = 0;
      if (limit_type == 0x01) {
        auto [parsed_max_size, max_bytes_read] = decode_uleb128(s, base_offset);
        max_size = parsed_max_size;
        base_offset += max_bytes_read;
      }

      if (DEBUG_TABLE_SECTION) {
        cout << "Table limits - Min size: " << min_size;
        if (limit_type == 0x01) {
          cout << ", Max size: " << max_size;
        }
        cout << endl;
      }

      TableInfo table_info;
      table_info.elem_type = elem_type;
      table_info.min_size = min_size;
      table_info.has_max = (limit_type == 0x01);
      table_info.max_size = max_size;

      tableInfoVec.push_back(table_info);
    }
  }
  void parse_element() {
    // element section
    // element section is used to set table contents
    auto [elem_count, bytes_read] = decode_uleb128(s, 0);
    uint64_t base_offset = bytes_read;

    cout << "Decoding element section: " << s.substr(0, length * 2) << endl;
    cout << "Total element count: " << elem_count << endl;
    for (int i = 0; i < elem_count; ++i) {
      auto [table_index, table_index_bytes_read] = decode_uleb128(s, base_offset);
      base_offset += table_index_bytes_read;

      // check offset type (should be 0x41, which is i32.const)
      string expr_opcode = s.substr(base_offset, 2);
      base_offset += 2;

      if (expr_opcode != "41") {
        cout << "Warning: Expected i32.const (0x41) for element offset expression, got: " << expr_opcode << endl;
      }

      auto [offset, offset_bytes_read] = decode_sleb128(s, base_offset);
      base_offset += offset_bytes_read;

      string end_opcode = s.substr(base_offset, 2);
      base_offset += 2;

      if (end_opcode != "0b") {
        cout << "Warning: Expected end opcode (0x0B) for element offset expression, got: " << end_opcode << endl;
      }

      auto [func_indices_count, func_indices_count_bytes_read] = decode_uleb128(s, base_offset);
      base_offset += func_indices_count_bytes_read;

      ElementSegment elem_segment;
      elem_segment.table_index = table_index;
      elem_segment.offset = offset;

      if (DEBUG_ELEMENT_SECTION) {
        cout << "--- Element segment " << i << " ---" << endl;
        cout << "Table index: " << table_index << endl;
        cout << "Offset: " << offset << endl;
        cout << "Function indices count: " << func_indices_count << endl;
        cout << "Function indices: ";
      }

      for (int j = 0; j < func_indices_count; ++j) {
        auto [func_idx, func_idx_bytes_read] = decode_uleb128(s, base_offset);
        base_offset += func_idx_bytes_read;

        elem_segment.function_indices.push_back(func_idx);
        cout << func_idx << " ";
      }
      cout << endl;
      elementSegments.push_back(elem_segment);
    }
    buildFunctionIndexTable(); // we can now use element section info to build the table to translate table index to function index
  }
  void global_var_initializer(uint64_t global_init_type, int64_t global_init_value, int i) {
    /**
     * Possible ways to share variables across functions:
     * Use C++ to allocate a space and pass it in, like wasm_stack
     * But we need a way to determine variable type for global variables
     *
     * Also, maybe the initialization can be done with C++? maybe no need to use assembly?
     */
    char *memPointer = globalMemory.get() + i * 8;
    if (global_init_type == 0x41) { // i32.const
      int32_t value = static_cast<int32_t>(global_init_value);
      memcpy(memPointer, &value, sizeof(int32_t));
      globalTypeGetter[i] = W_REG;
      if (DEBUG_GLOBAL_SECTION) {
        cout << "Initialized with i32.const: " << value << endl;
      }
    } else if (global_init_type == 0x42) { // i64.const
      int64_t value = static_cast<int64_t>(global_init_value);
      memcpy(memPointer, &value, sizeof(int64_t));
      globalTypeGetter[i] = X_REG;
      if (DEBUG_GLOBAL_SECTION) {
        cout << "Initialized with i64.const: " << value << endl;
      }
    } else if (global_init_type == 0x43) { // f32.const
      throw "f32.const not implemented yet";
    } else if (global_init_type == 0x44) { // f64.const
      throw "f64.const not implemented yet";
    } else if (global_init_type == 0x23) { // global.get
      // WARN: NOT COVERED BY TEST CASES YET!!
      auto regType = globalTypeGetter[i];
      size_t size_info = (regType == X_REG) ? 8 : 4;
      memcpy(memPointer, globalMemory.get() + global_init_value * 8, size_info);
      globalTypeGetter[i] = regType;
      if (DEBUG_GLOBAL_SECTION) {
        cout << "Initialized with global.get " << global_init_value << endl;
      }
    } else {
      cout << "Unknown global initialization type: " << global_init_type << endl;
    }
  }
  void parse_global() {
    // global section
    // Reference: https://github.com/sunfishcode/wasm-reference-manual/blob/master/WebAssembly.md#global-section
    // The Global Section consists of an array of global declarations.
    auto [global_count, bytes_read] = decode_uleb128(s, 0);
    uint64_t base_offset = bytes_read;
    cout << "Decoding global section: " << s.substr(0, length * 2) << endl;
    cout << "Total global count: " << global_count << endl;

    // allocate memory for global variables
    globalMemory.reset(new char[global_count * 8]()); // use 8 byte for i32, i64 regardless of its type

    for (int i = 0; i < global_count; ++i) {
      /**
       * A global declaration consists of:
        desc | global description | a description of the global variable
        init | instantiation-time initializer | the initial value of the global variable
       */
      // desc part
      // Global Description
      // type | value type | the type of the global variable
      // mutability | varuint1 | 0 if immutable, 1 if mutable
      auto [global_type, bytes_read_type] = decode_uleb128(s, base_offset); // probably no need to use leb128 decode here
      base_offset += bytes_read_type;
      auto [global_mutability, bytes_read_mutability] = decode_uleb128(s, base_offset);
      base_offset += bytes_read_mutability;
      if (DEBUG_GLOBAL_SECTION) {
        cout << "--- Info for global " << i << " ---" << endl;
        cout << "Type is: " << type_encodings.at(global_type) << endl;
        cout << "Mutability is: " << global_mutability << endl;
      }
      // init part
      // An instantiation-time initializer is a single instruction, which is one of the following:
      // const (of any type).
      // global.get

      auto [global_init_type, bytes_read_init_type] = decode_uleb128(s, base_offset);
      base_offset += bytes_read_init_type;

      auto [global_init_value, bytes_read_init_value] = decode_sleb128(s, base_offset);
      base_offset += bytes_read_init_value;

      global_var_initializer(global_init_type, global_init_value, i);
      auto [global_init_expr_end_end, bytes_read_init_expr_end_end] = decode_uleb128(s, base_offset);
      base_offset += bytes_read_init_expr_end_end;
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
    // TODO: refactor this into a function
    const string instructions =
        wasmFunctionVec[i].prep_sp_instr + wasmFunctionVec[i].init_param_instr + wasmFunctionVec[i].init_local_instr +
        wasmFunctionVec[i].wasm_instructions.substr(wasmFunctionVec[i].jit_begin, wasmFunctionVec[i].jit_end - wasmFunctionVec[i].jit_begin) +
        wasmFunctionVec[i].restore_sp_instr + encodeReturn(30, true, false);
    char *functionAddr = reinterpret_cast<char *>(symbol_table[i]);
    const size_t arraySize = instructions.length() / 2;
    for (size_t j = 0; j < arraySize; j++) {
      const string byteStr = instructions.substr(j * 2, 2);
      functionAddr[j] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    __builtin___clear_cache(functionAddr, functionAddr + arraySize);
  }
  // write address based on element section (with offset!) from symbol table to in_assembly_call_table
  void writeTable() {
    in_assembly_call_table = calloc(2048, sizeof(int));
    for (int i = 0; i < elementSegments.size(); ++i) {
      auto curElemSegment = elementSegments[i];
      if (curElemSegment.table_index != 0) {
        throw "multiple function table not supported yet";
      }
      for (int j = 0; j < curElemSegment.function_indices.size(); ++j) {
        void *funcAddr = symbol_table[curElemSegment.function_indices[j]];
        memcpy(reinterpret_cast<char *>(in_assembly_call_table) + (curElemSegment.offset + j) * sizeof(void *), &funcAddr, sizeof(void *));
      }
    }
    for (int i = 0; i < wasmFunctionVec.size(); ++i) {
      wasmFunctionVec[i].in_assembly_call_table = in_assembly_call_table;
    }
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
  // merge type with same structure
  void computeTypeEquivalence() {
    typeEquivalenceMap.resize(wasmFunctionTypeVec.size());
    for (size_t i = 0; i < wasmFunctionTypeVec.size(); ++i) {
      typeEquivalenceMap[i] = i;
    }
    map<string, size_t> signatureToCanonicalType;
    for (size_t i = 0; i < wasmFunctionTypeVec.size(); ++i) {
      string signature = generateTypeSignature(wasmFunctionTypeVec[i]);
      if (signatureToCanonicalType.find(signature) != signatureToCanonicalType.end()) {
        typeEquivalenceMap[i] = signatureToCanonicalType[signature];
      } else {
        signatureToCanonicalType[signature] = i;
      }
    }
  }
  // generate a signature to help identity types with different id but share same structure, we will merge them into same "base" type
  string generateTypeSignature(const WasmFunctionType &type) {
    string signature = "Params:";
    for (const auto &param : type.param_data) {
      signature += reg_char_map.at(getWasmType(param)) + ";";
    }
    signature += "Results:";
    for (const auto &result : type.result_data) {
      signature += reg_char_map.at(getWasmType(result)) + ";";
    }
    return signature;
  }

  void buildFunctionIndexTable() {
    size_t max_table_size = 0;
    // get max size first
    for (const auto &segment : elementSegments) {
      size_t segment_end = segment.offset + segment.function_indices.size();
      max_table_size = max(max_table_size, segment_end);
    }
    table_function_indices.resize(max_table_size, -1); // resize to max size, ensure there are enough space
    for (const auto &segment : elementSegments) {
      for (size_t i = 0; i < segment.function_indices.size(); ++i) {
        size_t table_idx = segment.offset + i;
        int func_idx = segment.function_indices[i];
        table_function_indices[table_idx] = func_idx;
      }
    }
  }

  void funcSingleProcess(int i) {
    cout << "------ Processing function " << i << ": " << funcIndexNameMapper[i] << " ------" << endl;
    wasmFunctionVec[i].symbol_table = symbol_table;
    wasmFunctionVec[i].tableInfoVec = tableInfoVec;
    wasmFunctionVec[i].typeEquivalenceMap = typeEquivalenceMap;
    wasmFunctionVec[i].globalMemory = globalMemory.get();
    wasmFunctionVec[i].globalTypeGetter = globalTypeGetter;
    wasmFunctionVec[i].generatePreWasmInstructions();
    wasmFunctionVec[i].processCodeVec();
    writeTable();
    writeCodeToMemory(i);
    wasmFunctionVec[i].table_function_indices = table_function_indices;
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
    computeTypeEquivalence();
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
  vector<TableInfo> tableInfoVec;               // store Table info, currently there should be only one table
  vector<ElementSegment> elementSegments;       // Table initializers are sometimes called "segments".
  vector<WasmFunction> wasmFunctionVec;         // used to store function code
  vector<WasmFunctionType> wasmFunctionTypeVec; // used to store type definition
  vector<int> wasmFunctionToTypeMapper;         // map function id to wasmType
  vector<int> table_function_indices;           // convert table index to function index

  map<string, int> funcNameIndexMapper; // function name to index
  map<int, string> funcIndexNameMapper; // function index to name

  map<int, void *> symbol_table;
  void *in_assembly_call_table;      // used to store call_indirect address
  vector<size_t> typeEquivalenceMap; // classify type with same structure into same id, this is used for signature verify currently

  unique_ptr<char[]> globalMemory;              // used to store global variables, globalMemory[i] = *(globalMemory + i*8)
  unordered_map<int, RegType> globalTypeGetter; // this does what it says
};