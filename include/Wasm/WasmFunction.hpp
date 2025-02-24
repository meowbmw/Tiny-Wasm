#pragma once
#include "CommonHeader.hpp"

const uint8_t REG_BUFFER = 19;
const uint8_t REG_WASM_STACK = 20;
const uint8_t REG_POINTER_WASM_STACK = 21;

class WasmFunction {
public:
  void processCodeVec() {
    int offset = 0;
    local_var_initialize(offset); // doesn't modify wasm_instructions
    printOriginWasmOpcode(offset);

    // start processing wasm_instructions here
    fakeInsertBranch("entry", "b"); // b main

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
    wasm_instructions += encodeMovz(11, 0x1, X_REG, 0);                     // x11=1, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, REG_BUFFER, 0); // [x[REG_BUFFER]]=x11=1
    fakeInsertBranch("finalize", "b");

    insertLabel("entry");
    main_entry_initialize(offset);

    wrapper_setjmp();
    jiting_wasm_code(offset);

    // store return code 0 to x[0]
    wasm_instructions += encodeMovz(11, 0x0, X_REG, 0);                     // x11=0, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, REG_BUFFER, 0); // [x[REG_BUFFER]]=x11=0

    insertLabel("finalize");
    main_entry_finalize();
    streambuf *old = cout.rdbuf();
    cout.rdbuf(0);
    fixUpfakeBranch();
    cout.rdbuf(old);
  }
  void printOriginWasmOpcode(int &offset);
  void initParam();
  void initLocal();
  void local_var_initialize(int &offset) {
    for (int i = 0; i < local_var_declare_count; ++i) {
      const unsigned int var_count_in_this_declare = stoul(code_vec[offset], nullptr, 16);
      const string var_type_in_this_declare = code_vec[offset + 1];
      for (int j = 0; j < var_count_in_this_declare; ++j) {
        add_data(TypeCategory::LOCAL, var_type_in_this_declare);
      }
      offset += 2;
    }
    // print_data(TypeCategory::PARAM);
    // print_data(TypeCategory::LOCAL);
  }
  void main_entry_initialize(int &offset) {
    getStackPreallocateSize(offset);
    prepareSp();
    initParam(); // initParam is storing to memory, prepareParams is storing to registers
    initLocal();
    // printInitStack();
  }
  void main_entry_finalize() {
    restoreSP();
    emitRet();
  }
  void allocateVar(const wasm_type &elem, int &stack_location) {
    std::visit(
        [&stack_location](auto &&value) {
          char typeInfo = typeid(value).name()[0];
          if (typeInfo == 'f') {
            stack_location += 4;
          } else if (typeInfo == 'd') {
            stack_location += 8;
          } else if (typeInfo == 'l') {
            stack_location += 4;
          } else if (typeInfo == 'i') {
            stack_location += 4;
          }
        },
        elem);
  }
  void getStackPreallocateSize(const int offset);
  void prepareSp();
  void printInitStack();
  void restoreSP();
  void prepareParams() {
    /**
     * Store params to their respective location before calling our function
     * param_data[0] -> x0
     * param_data[1] -> x1
     */
    cout << "--- Loading params to their respective registers ---" << endl;
    if (param_data.size() == 0) {
      cout << "No params need to be load" << endl;
    }
    cout << "Backing up x0 buffer to x" << +REG_BUFFER << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, REG_BUFFER, 0);
    cout << "Backing up x1 wasm_stack pointer to x" << +REG_WASM_STACK << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, REG_WASM_STACK, 1);

    // initialize both stack pointer with 0
    cout << "Initialze REG_POINTER_WASM_STACK: x" << +REG_POINTER_WASM_STACK << " with 0" << endl;
    pre_instructions_for_param_loading += encodeMovz(REG_POINTER_WASM_STACK, 0, X_REG);

    cout << "Loading parameters" << endl;
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
  void resetAfterExecution() {
    pre_instructions_for_param_loading.clear();
    wasm_instructions.clear(); // no need to reset this?
    fake_insert_map.clear();
    label_map.clear();
  }
  void wrapper_setjmp() {
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
  template <typename Func> auto getFunctionPointer(string full_instructions) -> Func;
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
  void commonLocalOp(int i, string opType) {
    u_int64_t var_index = stoul(code_vec[i + 1], nullptr, 16);
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
  void emitIfOp(int i);
  void emitElseOp();
  void emitEndOp();
  void emitRet();
  string push(RegType regType, int reg = 11);
  string pop(RegType regType, bool tee, int reg = 11);
  void constructFullinstr(string sub_instr);
  void jiting_wasm_code(int i) {
    cout << "--- JITing wasm code ---" << endl;
    control_flow_stack.push_back(
        controlFlowElement("end", result_data)); // TODO: this might need to be called on every function enter, currently it is only executed once.
    // This instruction is necessary for "end" to pop off control stack
    while (i < code_vec.size()) {
      /**
       * WebAssembly Opcodes
       * https://pengowray.github.io/wasm-ops/
       */
      if (code_vec[i] == "01") {
        // nop
        wasm_instructions += encodeNop();
        i += 1;
      } else if (code_vec[i] == "04") { // if
        // todo: currently don't support multi-value return
        // we assume at most one return can occur, or simply none return
        emitIfOp(i + 1);
        i += 2;
      } else if (code_vec[i] == "05") { // else
        emitElseOp();
        i += 1;
      } else if (code_vec[i] == "0f") { // ret
        i += 1;
      } else if (code_vec[i] == "0b") { // end
        emitEndOp();
        i += 1;
      } else if (code_vec[i] == "20") { // local.get
        commonLocalOp(i, "get");
        i += 2;
      } else if (code_vec[i] == "21") { // local.set
        commonLocalOp(i, "set");
        i += 2;
      } else if (code_vec[i] == "22") { // local.tee
        commonLocalOp(i, "tee");
        i += 2;
      } else if (code_vec[i] == "41") { // i32.const
        wasm_type elem = static_cast<int32_t>(stoul(code_vec[i + 1], nullptr, 16));
        // todo: read 1 byte is wrong here, should read by leb128 until end
        emitConst(elem);
        i += 2;
      } else if (code_vec[i] == "42") { // i64.const
        wasm_type elem = static_cast<int64_t>(stoul(code_vec[i + 1], nullptr, 16));
        // todo: read 1 byte is wrong here, should read by leb128 until end
        emitConst(elem);
        i += 2;
      } else if (code_vec[i] == "43") { // f32.const
        wasm_type elem = hexToFloat(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4]);
        emitConst(elem);
        i += 5;
      } else if (code_vec[i] == "44") { // f64.const
        wasm_type elem =
            hexToDouble(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4] + code_vec[i + 5] + code_vec[i + 6] + code_vec[i + 7]);
        emitConst(elem);
        i += 9;
      }
      // integer comparsion
      else if (code_vec[i] == "46") { // i32.eq
        emitCompareOp(W_REG, "eq");
        i += 1;
      } else if (code_vec[i] == "51") { // i64.eq
        emitCompareOp(X_REG, "eq");
        i += 1;
      } else if (code_vec[i] == "47") { // i32.ne
        emitCompareOp(W_REG, "ne");
        i += 1;
      } else if (code_vec[i] == "52") { // i64.ne
        emitCompareOp(X_REG, "ne");
        i += 1;
      } else if (code_vec[i] == "48") { // i32.lt_s
        emitCompareOp(W_REG, "lt");
        i += 1;
      } else if (code_vec[i] == "49") { // i32.lt_u
        emitCompareOp(W_REG, "cc");
        i += 1;
      } else if (code_vec[i] == "4a") { // i32.gt_s
        emitCompareOp(W_REG, "gt");
        i += 1;
      } else if (code_vec[i] == "4b") { // i32.gt_u
        emitCompareOp(W_REG, "hi");
        i += 1;
      } else if (code_vec[i] == "4c") { // i32.le_s
        emitCompareOp(W_REG, "le");
        i += 1;
      } else if (code_vec[i] == "4d") { // i32.le_u
        emitCompareOp(W_REG, "ls");
        i += 1;
      } else if (code_vec[i] == "4e") { // i32.ge_s
        emitCompareOp(W_REG, "ge");
        i += 1;
      } else if (code_vec[i] == "4f") { // i32.ge_u
        emitCompareOp(W_REG, "cs");
        i += 1;
      } else if (code_vec[i] == "53") { // i64.lt_s
        emitCompareOp(X_REG, "lt");
        i += 1;
      } else if (code_vec[i] == "54") { // i64.lt_u
        emitCompareOp(X_REG, "cc");
        i += 1;
      } else if (code_vec[i] == "55") { // i64.gt_s
        emitCompareOp(X_REG, "gt");
        i += 1;
      } else if (code_vec[i] == "56") { // i64.gt_u
        emitCompareOp(X_REG, "hi");
        i += 1;
      } else if (code_vec[i] == "57") { // i64.le_s
        emitCompareOp(X_REG, "le");
        i += 1;
      } else if (code_vec[i] == "58") { // i64.le_u
        emitCompareOp(X_REG, "ls");
        i += 1;
      } else if (code_vec[i] == "59") { // i64.ge_s
        emitCompareOp(X_REG, "ge");
        i += 1;
      } else if (code_vec[i] == "5a") { // i64.ge_u
        emitCompareOp(X_REG, "cs");
        i += 1;
      }
      // arithmetic
      else if (code_vec[i] == "6a") { // i32.add
        emitArithOp('i', '+');
        i += 1;
      } else if (code_vec[i] == "6b") { // i32.sub
        emitArithOp('i', '-');
        i += 1;
      } else if (code_vec[i] == "6c") { // i32.mul
        emitArithOp('i', '*');
        i += 1;
      } else if (code_vec[i] == "6d") { // i32.div_s
        emitArithOp('i', '/', true);
        i += 1;
      } else if (code_vec[i] == "6e") { // i32.div_u
        emitArithOp('i', '/', false);
        i += 1;
      } else if (code_vec[i] == "7c") { // i64.add
        emitArithOp('l', '+');
        i += 1;
      } else if (code_vec[i] == "7d") { // i64.sub
        emitArithOp('l', '-');
        i += 1;
      } else if (code_vec[i] == "7e") { // i64.mul
        emitArithOp('l', '*');
        i += 1;
      } else if (code_vec[i] == "7f") { // i64.div_s
        emitArithOp('l', '/', true);
        i += 1;
      } else if (code_vec[i] == "80") { // i64.div_u
        emitArithOp('l', '/', false);
        i += 1;
      }
    }
  }
  WasmFunction();
  // data section
  int stack_size = 0;
  int param_stack_start_location = 0;
  int param_stack_end_location = 0;
  int local_stack_start_location = 0;
  int local_stack_end_location = 0;
  int wasm_stack_start_location = 0;
  int wasm_stack_end_location = 0;
  int if_label = 0;
  int type;
  u_int64_t local_var_declare_count = 0;

  string wasm_instructions;
  string pre_instructions_for_param_loading;

  vector<string> code_vec;
  vector<wasm_type> local_data;
  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
  vector<controlFlowElement> control_flow_stack;

  map<char, ArithOperation> operations_map;
  map<pair<TypeCategory, int>, int> vecToStack;        // {TypeCategory::PARAM, 0} : 0x4
  map<pair<TypeCategory, int>, RegType> regTypeGetter; // {TypeCategory::PARAM, 0}: LDR32
  map<int, pair<TypeCategory, int>> stackToVec;        // 0x4 : {TypeCategory::PARAM: 0}
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