#pragma once
#include "WasmFunctionType.hpp"

const uint8_t reg_buffer = 19;
const uint8_t reg_wasm_stack = 20;
const uint8_t reg_pointer_wasm_stack = 21;
const uint8_t reg_pointer_globalvars = 22;
const uint8_t reg_pointer_wasm_memory = 23;
const uint8_t reg_memory_size = 24;
const uint8_t reg_pointer_memcpy = 25;
const uint8_t reg_max_memory_size = 26;
const uint8_t reg_min_allowed_sp_value = 27;

int max_allowed_size = 8192 * 2; // 8192 is needed to pass ch08 test cases, this value is customary

const bool enable_exception_handling = true;

struct TableInfo {
  string elem_type;  // element type, currently can only be "70"（funcref）
  uint64_t min_size; // minimal table size
  bool has_max;      // flag for if has max size
  uint64_t max_size; // max table size (if has) max
};

struct ElementSegment {
  uint64_t table_index; // expected to be 0 (only 1 table)
  int64_t offset;       // offset in table
  vector<uint64_t> function_indices;
};

struct DataInitializer {
  int memory_index = 0;
  int64_t data_offset = 0; // used to hold where to start writing wasm memory
  vector<char> data_vec;   // used to store values in this data initializer, i.e. "Hello world" (type is not limited to char though)
};

struct MemoryInfo {
  int max_page = 65535;
  int min_page = 1;
};
class WasmFunction {
public:
  void processCodeVec() {
    int offset = 0;
    local_var_initialize(offset); // doesn't modify wasm_instructions
    printOriginWasmOpcode(offset);

    getStackPreallocateSize(offset);

    cout << commonIndentString + "Getting minimum allowed sp" << endl;
    // backup origin sp
    wasm_instructions += encodeMovSP(X_REG, reg_min_allowed_sp_value, 31);
    // get minimum allowed sp
    wasm_instructions += WrapperEncodeMovInt32(10, max_allowed_size);
    wasm_instructions += encodeAddSubShift(true, X_REG, reg_min_allowed_sp_value, reg_min_allowed_sp_value, 10);

    prepareSp();
    jit_begin = wasm_instructions.size();

    // start processing wasm_instructions here
    fakeInsertBranch("entry", "b"); // b main

    if (enable_exception_handling) {
      injectExceptionHandling();
    }

    insertLabel("entry");
    main_entry_initialize(offset);

    if (enable_exception_handling) {
      enable_setjmp();
    }
    jiting_wasm_code(offset);

    jit_end = wasm_instructions.size();

    cout << "Store return code:" << endl;
    // store return code 0 to x[0]
    wasm_instructions += encodeMovz(X_REG, 11, 0x0, 0);                     // x11=0, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, reg_buffer, 0); // [x[reg_buffer]]=x11=0

    insertLabel("Finalize");
    getResult();
    restoreSP();
    wasm_instructions += encodeReturn();
    streambuf *old = cout.rdbuf();
    cout.rdbuf(0);
    fixUpfakeBranch();
    cout.rdbuf(old);
  }
  void printOriginWasmOpcode(int &offset);
  void injectExceptionHandling() {
    insertLabel("setjmp");
    wasm_instructions += getSetJmpInstr();

    insertLabel("preparelongjmp");
    wasm_instructions += encodeMovRegister(X_REG, 0, reg_buffer); // x0 <- x[reg_buffer]
    fakeInsertBranch("longjmp", "b");

    insertLabel("longjmp");
    wasm_instructions += getLongJmpInstr();

    insertLabel("raiseException");

    // store return code 1 to [reg_buffer]
    // todo: generate different return code based on exception type!
    wasm_instructions += encodeMovz(X_REG, 11, 0x1, 0);                     // x11=1, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, reg_buffer, 0); // [x[reg_buffer]]=x11=1
    fakeInsertBranch("Finalize", "b");
  }
  void initParam();
  void initLocal();
  void local_var_initialize(int &offset) {
    for (int i = 0; i < local_var_declare_count; ++i) {
      auto [var_count_in_this_declare, bytesread] = decode_uleb128_from_vec(code_vec, offset);
      const string var_type_in_this_declare = code_vec[offset + bytesread];
      for (int j = 0; j < var_count_in_this_declare; ++j) {
        add_data(TypeCategory::LOCAL, var_type_in_this_declare);
      }
      offset += 2;
    }
    // print_data(TypeCategory::PARAM);
    // print_data(TypeCategory::LOCAL);
  }
  void main_entry_initialize(int &offset) {
    initParam(); // initParam is storing to memory, generatePreWasmInstructions is storing to registers
    initLocal();
    // printInitStack();
  }
  void allocateVar(const wasm_type &elem, int &stack_location) {
    std::visit(
        [&stack_location](auto &&value) {
          char typeInfo = typeid(value).name()[0];
          if (typeInfo == 'f') {
            stack_location += 8;
          } else if (typeInfo == 'd') {
            stack_location += 8;
          } else if (typeInfo == 'l') {
            stack_location += 8;
          } else if (typeInfo == 'i') {
            stack_location += 8;
          }
        },
        elem);
  }
  void getStackPreallocateSize(const int offset);
  void prepareSp();
  void printInitStack();
  void getResult();
  void restoreSP();
  /**
   * @brief Prepares function parameters for execution in the WASM runtime
   *
   * This function handles the setup necessary before executing a WASM function:
   * 1. Backs up critical registers (x0 buffer and x1 WASM stack pointer)
   * 2. Initializes the WASM stack pointer register (reg_pointer_wasm_stack) to 0
   * 3. Loads all parameters into their appropriate registers according to their types
   *
   * The function generates all necessary instructions and appends them to the
   * pre_instructions_for_param_loading string, which will be executed before the
   * function body.
   */
  void generatePreWasmInstructions() {
    cout << "--- Loading params to their respective registers ---" << endl;
    if (param_data.size() == 0) {
      cout << "No params need to be load" << endl;
    }
    cout << "Backing up x0 buffer to x" << +reg_buffer << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, reg_buffer, 0);
    cout << "Backing up x1 wasm_stack pointer to x" << +reg_wasm_stack << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, reg_wasm_stack, 1);
    cout << "Backing up x2 global variable pointer to x" << +reg_pointer_globalvars << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, reg_pointer_globalvars, 2);
    cout << "Backing up x3 memory size pointer to x" << +reg_memory_size << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, reg_memory_size, 3);
    cout << "Backing up x5 memcpy pointer to x" << +reg_pointer_memcpy << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, reg_pointer_memcpy, 5);
    cout << "Backing up x6 max memory size to x" << +reg_max_memory_size << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, reg_max_memory_size, 6);

    // initialize stack pointer with 0
    cout << "Initialze reg_pointer_wasm_stack: x" << +reg_pointer_wasm_stack << " with 0" << endl;
    pre_instructions_for_param_loading += encodeMovz(X_REG, reg_pointer_wasm_stack, 0);

    // initialize memory, this should only be executed once, we use [x3] to store memory size
    // memory initialization function will be stored in x4
    // ! only call memory initializer if there is a memory initialization function
    if (VecMemInfo.size() > 0 && VecMemInfo[0].min_page > 0) {
      // 我们规定reg_memory_size里面存的是页数，在计算具体内存地址的时候*65536
      cout << "--- Memory Initializer ---" << endl;
      pre_instructions_for_param_loading += encodeLoadStoreImm(W_REG, LDR, 11, reg_memory_size, 0); // r11=[current memory size]
      pre_instructions_for_param_loading += encodeCompareImm(W_REG, 11, 0);
      // skip initialization if memory size is not 0 (it should be 0 for the first time)
      pre_instructions_for_param_loading += encodeBranchCondition(6, reverse_cond_str_map.at("ne"));
      pre_instructions_for_param_loading += encodeMovRegister(X_REG, 14, 30); // backup x30 before blr
      pre_instructions_for_param_loading += encodeBranchRegister(4, true);
      pre_instructions_for_param_loading += encodeMovRegister(X_REG, 30, 14); // restore x30
      // load memory size to w11
      pre_instructions_for_param_loading += WrapperEncodeMovInt32(11, VecMemInfo[0].min_page);
      // set memory size
      pre_instructions_for_param_loading += encodeLoadStoreImm(W_REG, STR, 11, reg_memory_size, 0);
    }

    cout << "---Loading parameters---" << endl;
    for (int i = 0; i < param_data.size(); ++i) {
      std::visit(
          [&i, this](auto &&value) {
            char typeInfo = typeid(value).name()[0];
            if (typeInfo == 'f') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'd') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'l') {
              const string instr = WrapperEncodeMovInt64(i, value);
              pre_instructions_for_param_loading += instr;
            } else if (typeInfo == 'i') {
              const string instr = WrapperEncodeMovInt32(i, value);
              pre_instructions_for_param_loading += instr;
            }
          },
          param_data[i]);
    }
    cout << "---Loading parameters finished---" << endl;
  }
  void clear() {
    pre_instructions_for_param_loading.clear();
    wasm_instructions.clear(); // no need to reset this?
    fake_insert_map.clear();
    label_map.clear();
  }
  void enable_setjmp() {
    cout << "Setting up setjmp" << endl;

    wasm_instructions += encodeMovRegister(X_REG, 0, reg_buffer); // x0 <- x[reg_buffer]
    fakeInsertBranch("setjmp", "bl");                             // bl setjmp
    // 一旦bl进setjmp后x30的值就被更改了，就会导致丢失先前的x30值，所以必须要有一种机制去做备份和还原
    // 不管是进入setjmp之前还是setjmp之后的x30都有必要去做备份

    wasm_instructions += encodeCompareImm(X_REG, 0, 0);
    fakeInsertBranch("raiseException", "bne"); // if not equal, goto exception handling
  }
  void fakeInsertBranch(string label, string BranchStr);
  void insertLabel(string label);
  void fixUpfakeBranch();
  int64_t executeWasmInstr();
  void print_data(TypeCategory category);
  void add_data(TypeCategory category, const std::string &type) {
    wasm_type data;
    if (type == "7f") {
      data = static_cast<int32_t>(0);
    } else if (type == "7e") {
      data = static_cast<int64_t>(0);
    } else if (type == "7d") {
      data = static_cast<float>(0);
    } else if (type == "7c") {
      data = static_cast<double>(0);
    } else {
      std::cerr << "Adding data failed. Unknown data type: " << type << std::endl;
      return;
    }
    if (category == TypeCategory::PARAM) {
      param_data.push_back(data);
    } else if (category == TypeCategory::RESULT) {
      result_data.push_back(data);
    } else if (category == TypeCategory::LOCAL) {
      local_data.push_back(data);
    }
  }
  void set_code_vec(vector<string> &v, size_t l = 0) {
    code_vec = v;
    local_var_declare_count = l;
  }
  WasmFunctionType getWasmFunctionType(int i) {
    return wasmFunctionTypeVec[wasmFunctionToTypeMapper[i]];
  }
  void commonLocalOp(uint64_t var_index, string opType) {
    cout << format("Local.{} {}", opType, var_index) << endl;
    TypeCategory typecategory;
    if (var_index < param_data.size() + local_data.size()) {
      if (var_index < param_data.size()) {
        // falls within boundry of param variable
        typecategory = TypeCategory::PARAM;
      } else {
        // must be local variable then.
        typecategory = TypeCategory::LOCAL;
        var_index -= param_data.size();
      }
      if (opType == "get") {
        emitGet(var_index, typecategory);
      } else if (opType == "set") {
        emitSet(var_index, typecategory);
      } else if (opType == "tee") {
        emitSet(var_index, typecategory, true);
      } else {
        cout << "Unknown Local operation" << endl;
        throw "Unknown Local operation";
      }
    } else {
      throw "Too big index {" + to_string(var_index) + "} for local data; skipping current op;";
    }
  }
  void commonLoadStoreOp(int &i, RegType regtype, LdStType ldstType, DataWidth datawidth, ExtendMode extendMode) {
    auto [alignment, alignment_bytes_read] = decode_uleb128_from_vec(code_vec, i + 1);
    i += alignment_bytes_read + 1; // WARN: aligement doesn't actually change anything so it will be discarded for now!!!
    auto [offset, offset_bytes_read] = decode_uleb128_from_vec(code_vec, i);
    i += offset_bytes_read;
    emitMemoryLoadStore(regtype, ldstType, datawidth, extendMode, offset);
  }
  void emitGet(const uint64_t var_to_get, TypeCategory vecType);
  void emitSet(const uint64_t var_to_set, TypeCategory vecType, bool isTee = false);
  void emitConst(wasm_type elem);
  void emitArithOp(char typeInfo, char opType, bool isSigned = true);
  void emitCompareOp(RegType regtype, string condStr);
  void emitCall(int function_index);
  void emitCallIndirect(size_t type_index, size_t table_index);
  void emitBlock(int i);
  void emitLoop(int i);
  void emitBr(int i);
  void emitBr_if(int i);
  void emitIfOp(int i);
  void emitElseOp();
  void emitEndOp();
  void emitReturnOp();
  void emitDrop();
  void emitClz(RegType regtype);
  void emitCtz(RegType regtype);
  void emitEqz(RegType regtype);
  void emitGlobalGet(uint64_t var_index);
  void emitGlobalSet(uint64_t var_index);
  void emitMemoryLoadStore(RegType regtype, LdStType ldstType, DataWidth datawidth, ExtendMode extendMode, uint32_t offset);
  void emitCheckMemoryBoundary(DataWidth datawidth);
  void emitMemorySize();
  void emitMemoryGrow();
  string push(RegType regType, int reg = 11);
  string pop(RegType regType, bool tee = false, int reg = 11);
  string allocateMemory();
  string allocateMemory(uint16_t page_size);
  string releaseMemory();
  string releaseMemory(uint16_t page_size);
  string growMemory();
  void constructFullinstr(string sub_instr);
  void jiting_wasm_code(int i);
  // data section
  int stack_size = 0;
  int param_stack_start_location = 0;
  int param_stack_end_location = 0;
  int local_stack_start_location = 0;
  int local_stack_end_location = 0;
  int if_label = 0;
  int block_label = 0;
  int loop_label = 0;
  int grow_label = 0;
  int type;
  u_int64_t local_var_declare_count = 0;
  const uint8_t called_function_register = 9;
  int jit_begin = 0;
  int jit_end = 0;
  int64_t data_offset = 0;

  string functionName;
  string wasm_instructions;
  string pre_instructions_for_param_loading;
  string prep_sp_instr;
  string restore_sp_instr;
  string init_local_instr;
  string init_param_instr;

  vector<string> code_vec;
  vector<wasm_type> local_data;
  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
  vector<WasmFunctionType> wasmFunctionTypeVec;
  vector<int> wasmFunctionToTypeMapper;
  vector<controlFlowElement> control_flow_stack;
  vector<TableInfo> tableInfoVec; // store Table info, currently there should be only one table
  vector<int> table_function_indices;

  map<pair<TypeCategory, int>, int> vecToStack;        // {TypeCategory::PARAM, 0} : 0x4
  map<pair<TypeCategory, int>, RegType> regTypeGetter; // {TypeCategory::PARAM, 0}: LDR32
  map<int, pair<TypeCategory, int>> stackToVec;        // 0x4 : {TypeCategory::PARAM: 0}
  map<int, void *> symbol_table;
  void *in_assembly_call_table;
  vector<size_t> typeEquivalenceMap; // classify type with same structure into same id, this is used for signature verify currently

  void *memoryInitializeFunction; //

  char *globalVars;                             // used to store global variables, globalVars[i] = *(globalVars + i*8)
  char *globalSizeArray;                        // used to store size of global variables, globalSizeArray[i] = *(globalSizeArray + i)
  unordered_map<int, RegType> globalTypeGetter; // this does what it says

  vector<MemoryInfo> VecMemInfo;
  void *memorySizeKeeper; // also used as a flag to check if memory has been initialized, should be pass in from WasmFile

  unordered_multimap<string, pair<int64_t, string>> fake_insert_map;
  unordered_map<string, int64_t> label_map;
  /**
   * we have 4 vectors
   * vector<int>, vector<double> ..
   * we have 5 variables, 1xint,2xdouble,1xlong,1xint
   * index is: 00,01,02,03,04
   * we need to be able to access by index
   * when adding to a vector, remeber its current index
   * like, index in locals 04, corresponding vector index 01
   */
};