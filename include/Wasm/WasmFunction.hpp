#pragma once
#include "WasmFunctionType.hpp"

const uint8_t REG_BUFFER = 19;
const uint8_t REG_WASM_STACK = 20;
const uint8_t REG_POINTER_WASM_STACK = 21;
const uint8_t REG_POINTER_GLOBAL_MEMORY = 22;

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
class WasmFunction {
public:
  void processCodeVec() {
    int offset = 0;
    local_var_initialize(offset); // doesn't modify wasm_instructions
    printOriginWasmOpcode(offset);

    getStackPreallocateSize(offset);
    prepareSp();
    jit_begin = wasm_instructions.size();

    // start processing wasm_instructions here
    fakeInsertBranch("entry", "b"); // b main

    injectExceptionHandling();

    insertLabel("entry");
    main_entry_initialize(offset);

    enable_setjmp();
    jiting_wasm_code(offset);

    jit_end = wasm_instructions.size();

    cout << "Store return code:" << endl;
    // store return code 0 to x[0]
    wasm_instructions += encodeMovz(11, 0x0, X_REG, 0);                     // x11=0, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, REG_BUFFER, 0); // [x[REG_BUFFER]]=x11=0

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
    wasm_instructions += encodeMovRegister(
        X_REG, 14, 30); // x14 <- x30, this is just backing up, x14 can be any other register that isn't used, same thing applies to x15 <- sp
    wasm_instructions += encodeMovSP(X_REG, 15, 31);              // x15 <- sp
    wasm_instructions += encodeMovRegister(X_REG, 0, REG_BUFFER); // x0 <- x[REG_BUFFER]
    fakeInsertBranch("longjmp", "b");

    insertLabel("longjmp");
    wasm_instructions += getLongJmpInstr();

    insertLabel("raiseException");
    // doing this because the sp we backed up is wrong, its after stp (sp is decreased)
    // also x30 is set to the instruction after bl setjmp, not the origin return address, so it's also wrong
    wasm_instructions += encodeMovRegister(X_REG, 30, 14); // x30 <- x14
    wasm_instructions += encodeMovSP(X_REG, 31, 15);       // sp <- x31

    // store return code 1 to [x13]
    // todo: generate different return code based on exception type!
    wasm_instructions += encodeMovz(11, 0x1, X_REG, 0);                     // x11=1, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, REG_BUFFER, 0); // [x[REG_BUFFER]]=x11=1
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
   * 2. Initializes the WASM stack pointer register (REG_POINTER_WASM_STACK) to 0
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
    cout << "Backing up x0 buffer to x" << +REG_BUFFER << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, REG_BUFFER, 0);
    cout << "Backing up x1 wasm_stack pointer to x" << +REG_WASM_STACK << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, REG_WASM_STACK, 1);
    cout << "Backing up x2 globalMemory to x" << +REG_POINTER_GLOBAL_MEMORY << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, REG_POINTER_GLOBAL_MEMORY, 2);

    // initialize stack pointer with 0
    cout << "Initialze REG_POINTER_WASM_STACK: x" << +REG_POINTER_WASM_STACK << " with 0" << endl;
    pre_instructions_for_param_loading += encodeMovz(REG_POINTER_WASM_STACK, 0, X_REG);

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
    wasm_instructions += encodeLdpStp(X_REG, STR, 29, 30, 31, -0x20, EncodingMode::PreIndex); // stp x29, x30, [sp, #-0x20]!

    wasm_instructions += encodeMovRegister(X_REG, 0, REG_BUFFER); // x0 <- x[REG_BUFFER]
    fakeInsertBranch("setjmp", "bl");                             // bl setjmp

    wasm_instructions += encodeCompareImm(X_REG, 0, 0);
    fakeInsertBranch("raiseException", "bne");                                                // todo: if not equal, goto exception handling
    wasm_instructions += encodeLdpStp(X_REG, LDR, 29, 30, 31, 0x20, EncodingMode::PostIndex); // ldp x29, x30, [sp], #0x20
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
  void emitCtz(RegType regtype);
  void emitEqz(RegType regtype);
  void emitGlobalGet(uint64_t var_index);
  void emitGlobalSet(uint64_t var_index);
  string push(RegType regType, int reg = 11);
  string pop(RegType regType, bool tee = false, int reg = 11);
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
  int type;
  u_int64_t local_var_declare_count = 0;
  const uint8_t called_function_register = 9;
  int jit_begin = 0;
  int jit_end = 0;

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

  char *globalMemory;                           // used to store global variables, globalMemory[i] = *(globalMemory + i*8)
  char *globalSizeArray;                        // used to store size of global variables, globalSizeArray[i] = *(globalSizeArray + i)
  unordered_map<int, RegType> globalTypeGetter; // this does what it says

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