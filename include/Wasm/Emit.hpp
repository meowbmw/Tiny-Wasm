#pragma once
#include "WasmFunction.hpp"

auto getSignature(string s) {
  vector<wasm_type> signature;
  if (s == "7f") { // i32
    signature.push_back(static_cast<int32_t>(0));
  } else if (s == "7e") { // i64
    signature.push_back(static_cast<int64_t>(0));
  } else if (s == "7d") { // f32
    throw "return f32 after if not implemented yet";
  } else if (s == "7c") { // f64
    throw "return f64 after if not implemented yet";
  } else if (s == "70") { // funcref
    throw "return funcref after if not implemented yet";
  } else if (s == "60") { // func
    throw "return func after if not implemented yet";
  } else if (s == "40") { // void
    // no need to push anything
  } else {
    throw format("invalid byte after if: {}. Check wasm binary integrity", s);
  }
  return signature;
}

void WasmFunction::emitGet(const uint64_t var_to_get, TypeCategory vecType) {
  /**
   * Local.get i
   * push to wasm stack memory[var[i]]
   * var[i] -> x/w11 -> stack[top]
   */
  RegType regtype = regTypeGetter[{vecType, var_to_get}];
  int stack_offset = vecToStack[{vecType, var_to_get}];
  cout << format("{}Getting {}[{}]", commonIndentString, type_category_to_string(vecType), var_to_get) << endl;
  // Note: We use x11 as a bridge register for memory -> memory transfer!
  // var[i] -> x/w11
  string load_param_instr = encodeLoadStoreImm(regtype, LDR, 11, 31, stack_offset);
  // x/w11 -> stack[top]
  string store_to_stack_instr = push(regtype);
  constructFullinstr(load_param_instr + store_to_stack_instr);
}
void WasmFunction::emitSet(const uint64_t var_to_set, TypeCategory vecType, bool isTee) {
  /**
   * Local.set i
   * Set memory[var[i]] to top value of wasm stack
   * stack[top] -> x/w11 -> var[i]
   */
  RegType regtype = regTypeGetter[{vecType, var_to_set}];
  int stack_offset = vecToStack[{vecType, var_to_set}];
  string load_to_reg_instr = pop(regtype, isTee);
  cout << format("{}Assigning to {}[{}]", commonIndentString, type_category_to_string(vecType), var_to_set) << endl;
  string reg_to_mem_instr = encodeLoadStoreImm(regtype, STR, 11, 31, stack_offset);
  constructFullinstr(load_to_reg_instr + reg_to_mem_instr);
}
void WasmFunction::emitGlobalGet(uint64_t var_index) {
  cout << "Global.get " << var_index << endl;
  RegType regType = globalTypeGetter[var_index];
  wasm_instructions += encodeLoadStoreImm(regType, LDR, 11, reg_pointer_globalvars, 8 * var_index); // load global variable to r11
  wasm_instructions += push(regType);                                                               // push r11 to wasm stack
}
void WasmFunction::emitGlobalSet(uint64_t var_index) {
  cout << "Global.set " << var_index << endl;
  RegType regType = globalTypeGetter[var_index];
  wasm_instructions += pop(regType);                                                                // pop from wasm stack to r11
  wasm_instructions += encodeLoadStoreImm(regType, STR, 11, reg_pointer_globalvars, 8 * var_index); // store r11 to global variable
}

void WasmFunction::emitCheckMemoryBoundary() {
  cout << format("{}*Checking Memory Boundary before memory read/write", commonIndentString) << endl;
  wasm_instructions += encodeLoadStoreImm(W_REG, LDR, 11, reg_memory_size, 0); // get current page size to w11
  wasm_instructions += WrapperEncodeMovInt32(10, 65536);                       // 65536 is how much bytes a page has
  wasm_instructions += encodeMul(W_REG, 11, 11, 10);                           // store current memory size(limit) in w11
  wasm_instructions += encodeCompareShift(W_REG, 11, 12);                      // limit <> address
  cout << format("{}if limit <= address, goes to longjmp", commonIndentString) << endl;
  fakeInsertBranch("preparelongjmp", "ble"); // throw if limit <= address
}

//  this function is used to read/write Wasm Memory
//  Memory.load/store
void WasmFunction::emitMemoryLoadStore(RegType regtype, LdStType ldstType, DataWidth datawidth, ExtendMode extendMode, uint32_t offset) {
  string typePrefix = (regtype == X_REG) ? "i64" : "i32";
  string opType = (ldstType == LDR) ? "load" : "store";
  string widthSuffix = "";
  string extendSuffix = "";

  switch (datawidth) {
  case DataWidth::byte:
    widthSuffix = "8";
    break;
  case DataWidth::word:
    widthSuffix = "16";
    break;
  default:
    break;
  }

  if (ldstType == LDR && !widthSuffix.empty()) {
    extendSuffix = (extendMode == ZeroExtend) ? "_u" : "_s";
  }

  cout << format("{}.{}{}{} offset={}", typePrefix, opType, widthSuffix, extendSuffix, offset) << endl;

  if (ldstType == LDR) {
    cout << format("{}Getting base", commonIndentString) << endl;
    wasm_instructions += pop(W_REG, false, 12); // pop base to w12
    cout << format("{}Adding offset to base", commonIndentString) << endl;
    wasm_instructions += encodeAddSubImm(W_REG, false, 12, 12, offset); // add offset to base
    emitCheckMemoryBoundary();
    cout << format("{}Loading memory", commonIndentString) << endl;
    wasm_instructions += commonLoadStoreReg(regtype, ldstType, datawidth, 11, reg_pointer_wasm_memory, 12, extendMode);
    wasm_instructions += push(regtype);
  } else {
    cout << format("{}Getting value", commonIndentString) << endl;
    wasm_instructions += pop(W_REG);                                    // pop value to w11
    cout << format("{}Getting base", commonIndentString) << endl;
    wasm_instructions += pop(W_REG, false, 12);                         // pop base to w12
    cout << format("{}Adding offset to base", commonIndentString) << endl;
    wasm_instructions += encodeAddSubImm(W_REG, false, 12, 12, offset); // add offset to base
    emitCheckMemoryBoundary();
    cout << format("{}Storing memory", commonIndentString) << endl;
    wasm_instructions += commonLoadStoreReg(regtype, ldstType, datawidth, 11, reg_pointer_wasm_memory, 12, extendMode);
  }
}

void WasmFunction::emitMemoryGrow() {
  wasm_instructions += pop(W_REG); // pop delta (pages to increase based on old size)
}

void WasmFunction::emitMemorySize() {
  wasm_instructions += encodeLoadStoreImm(W_REG, LDR, 11, reg_memory_size, 0);
  wasm_instructions += push(W_REG);
}

void WasmFunction::emitConst(wasm_type elem) {
  /***
   * push value $elem onto wasm Stack
   * mov $elem, x11
   * str x11, stack[top]
   */
  std::visit(
      [this](auto &&value) {
        char typeInfo = typeid(value).name()[0];
        if (typeInfo == 'f') {
          // todo: don't support fmov yet
          throw std::invalid_argument("Don't support float const yet; need to properly implement fmov or store float first");
        } else if (typeInfo == 'd') {
          throw std::invalid_argument("Don't support double const yet; need to properly implement fmov or store double first");
        } else if (typeInfo == 'i') {
          cout << format("i32.const {}", value) << endl;
          string load_to_reg_instr = WrapperEncodeMovInt32(11, value);
          string store_to_stack_instr = push(W_REG);
          constructFullinstr(load_to_reg_instr + store_to_stack_instr);
        } else if (typeInfo == 'l') {
          cout << format("i64.const {}", value) << endl;
          string load_to_reg_instr = WrapperEncodeMovInt64(11, value);
          string store_to_stack_instr = push(X_REG);
          constructFullinstr(load_to_reg_instr + store_to_stack_instr);
        }
      },
      elem);
}
void WasmFunction::emitCompareOp(RegType regtype, string condStr) {
  switch (regtype) {
  case X_REG:
    cout << "i64." << condCodeToWasmOp.at(condStr) << endl;
    break;
  case W_REG:
    cout << "i32." << condCodeToWasmOp.at(condStr) << endl;
    break;
  default:
    throw "Unsupported reg type (float or double)";
  }
  // todo: this could be simplified by one cset instruction
  wasm_instructions += encodeMovz(X_REG, 3, 1); // x3=1
  wasm_instructions += encodeMovz(X_REG, 4, 0); // x4=0
  // pop b
  string load_second_param_instr = pop(regtype, false, 12);
  // pop a
  string load_first_param_instr = pop(regtype, false, 11);
  // we should load second first, then load first!!!
  constructFullinstr(load_second_param_instr + load_first_param_instr);
  wasm_instructions += encodeCompareShift(regtype, 11, 12);                          // cmp a, b
  wasm_instructions += encodeCSEL(X_REG, 0, 3, 4, reverse_cond_str_map.at(condStr)); // x0 = (condStr) ? 1 : 0
  wasm_instructions += push(X_REG, 0);                                               // push result(x0) to stack
}
void WasmFunction::emitArithOp(char typeInfo, char opType, bool isSigned) {
  /*
   * A wrapper for common arithmatic operations: +, -, *, /
   */
  // Note: isSigned is only used to differentiate div_s and div_u
  RegType regtype;
  string opstr;
  switch (opType) {
  case '+':
    opstr = "add";
    break;
  case '-':
    opstr = "sub";
    break;
  case '*':
    opstr = "mul";
    break;
  case '/':
    opstr = "div";
    if (isSigned) {
      opstr += "_s";
    } else {
      opstr += "_u";
    }
    break;
  default:
    throw "Unknown arithmetic operator";
    break;
  }
  if (typeInfo == 'i') {
    regtype = W_REG;
    cout << format("i32.{}", opstr) << endl;
  } else if (typeInfo == 'l') {
    regtype = X_REG;
    cout << format("i64.{}", opstr) << endl;
  }
  // pop b to r11
  string load_second_param_instr = pop(regtype, false, 11);
  // pop a to r12
  string load_first_param_instr = pop(regtype, false, 12);
  constructFullinstr(load_second_param_instr + load_first_param_instr);
  // r11 = a op b
  string arith_instr;
  string check_div_instr;
  string branch_equal_zero_instr;
  switch (opType) {
  case '+':
    wasm_instructions += encodeAddSubShift(false, regtype, 11, 12, 11);
    break;
  case '-':
    wasm_instructions += encodeAddSubShift(true, regtype, 11, 12, 11);
    break;
  case '*':
    wasm_instructions += encodeMul(regtype, 11, 12, 11);
    break;
  case '/':
    wasm_instructions += encodeCompareImm(regtype, 11, 0); // cmp b, #0
    fakeInsertBranch("preparelongjmp", "beq");             // checks for division by zero

    // if b = -1, checks a
    if (regtype == X_REG) {
      wasm_instructions += WrapperEncodeMovInt64(5, -1);
      wasm_instructions += encodeCompareShift(X_REG, 11, 5);
      fakeInsertBranch("normal_div", "bne"); // if b != -1, goto normal div

      wasm_instructions += WrapperEncodeMovInt64(5, INT64_MIN);
      wasm_instructions += encodeCompareShift(X_REG, 12, 5);
      fakeInsertBranch("normal_div", "bne"); // if a != INT64_MIN, goto normal div
    } else {
      wasm_instructions += WrapperEncodeMovInt32(5, -1);
      wasm_instructions += encodeCompareShift(W_REG, 11, 5);
      fakeInsertBranch("normal_div", "bne"); // if b != -1, goto normal div

      wasm_instructions += WrapperEncodeMovInt32(5, INT32_MIN);
      wasm_instructions += encodeCompareShift(W_REG, 12, 5);
      fakeInsertBranch("normal_div", "bne"); // if a != INT32_MIN, goto normal div
    }
    fakeInsertBranch("preparelongjmp", "b"); // this means a=INT32_MIN and b=-1, goto raise exception int overflow

    insertLabel("normal_div");
    wasm_instructions += encodeDiv(regtype, isSigned, 11, 12, 11);

    break;
  default:
    throw "Unknown arithmetic operator";
    break;
  }
  wasm_instructions += push(regtype);
}
void WasmFunction::emitBlock(int i) {
  cout << "Block" << endl;
  vector<wasm_type> signature = getSignature(code_vec[i + 1]); // should be all zeros with different types
  string label = "Block end #" + to_string(block_label++);
  control_flow_stack.push_back(controlFlowElement(label, signature));
}
void WasmFunction::emitCall(int function_index) {
  cout << format("Call {}", function_index) << endl;
  cout << commonIndentString + "Loading parameters to register before calling" << endl;
  const WasmFunctionType v = getWasmFunctionType(function_index);

  for (int i = v.param_data.size() - 1; i >= 0; --i) {
    wasm_type t = v.param_data[i]; // popping reversely
    switch (getWasmType(t)) {
    case X_REG:
      wasm_instructions += pop(X_REG, false, i);
      break;
    case W_REG:
      wasm_instructions += pop(W_REG, false, i);
      break;
    default:
      throw "Float type unsupported yet";
      break;
    }
  }

  cout << format("{}Calling function index: {}", commonIndentString, function_index) << endl;
  wasm_instructions +=
      WrapperEncodeMovInt64(called_function_register, reinterpret_cast<uint64_t>(symbol_table[function_index])); // mov call_reg, function_address

  wasm_instructions += encodeBranchRegister(called_function_register, true); // blr call_reg
}
// type_index: used to get function signature (type)
// table_index: which table to use (should always be 0 for wasm 1.0, i.e. only 1 table)
void WasmFunction::emitCallIndirect(size_t type_index, size_t table_index) {
  cout << format("Call_indirect with type: {}, table: {}", type_index, table_index) << endl;

  // step 1: check Trap: Indirect Callee Absent, if the indexed table element is the special "null" value.
  // Check function index within range??
  cout << format("{}Check Trap: Indirect Callee Absent", commonIndentString) << endl;
  wasm_instructions += pop(W_REG, false, 11); // get indexed table element from stack, save it in w11, $callee: i32
  cout << format("{}Backup x11 because we will overwrite it in trap verification", commonIndentString) << endl;
  wasm_instructions += encodeMovRegister(X_REG, 16, 11);
  // check >=0
  wasm_instructions += encodeCompareImm(X_REG, 11, 0);
  cout << format("{}if type_index < 0, goes to longjmp", commonIndentString) << endl;
  fakeInsertBranch("preparelongjmp", "blt"); // if type_index < 0, goes to longjmp

  if (tableInfoVec[table_index].has_max) {
    // check < max_size
    wasm_instructions += encodeCompareImm(X_REG, 11, tableInfoVec[table_index].max_size);
    cout << format("{}if type_index >= max_size, goes to longjmp", commonIndentString) << endl;
    fakeInsertBranch("preparelongjmp", "bge"); // if type_index >= max_size, goes to longjmp
  }

  // step 2: check Trap: Indirect Call Type Mismatch, if the signature of the function with index $callee differs from the signature in the Type
  // Section with index $signature.
  // get $callee signature first
  /**
   * 类似于
      call_indirect (type 0) (i32.const 3)
      我去比较class(0) 和class(type(function 3))是否一致
      所以先获取实际类型，然后转换成通用类型（即结构一致即归为一类），然后再去比较通用类型
   */
  cout << format("{}Check Trap: Indirect Call Type Mismatch", commonIndentString) << endl;
  cout << format("{}Getting function index from table index", commonIndentString) << endl;
  wasm_instructions += WrapperEncodeMovInt64(10, reinterpret_cast<int64_t>(table_function_indices.data())); // x10 = table_function_indices
  wasm_instructions += encodeMovz(X_REG, 13, sizeof(int));                                                  // x13 = sizeof(int) = 4
  wasm_instructions += encodeMul(X_REG, 13, 11, 13);                                                        // x13 = x11 * x13
  wasm_instructions += encodeLoadStoreReg(W_REG, LDR, 11, 10, 13);                                          // w11 = [x10+x13]

  // todo: check w11 is not -1, currently can't compare because CompareImm doesn't support negative imm

  cout << format("{}Get real type", commonIndentString) << endl;
  wasm_instructions += WrapperEncodeMovInt64(10, reinterpret_cast<int64_t>(wasmFunctionToTypeMapper.data())); // x10=type array
  wasm_instructions += encodeMovz(X_REG, 13, sizeof(int));                                                    // x13=4
  wasm_instructions += encodeMul(X_REG, 13, 11, 13);                                                          // x13=i*4
  wasm_instructions += encodeLoadStoreReg(W_REG, LDR, 12, 10, 13);                                            // w12=[x10,i*4]=type array[i]

  cout << format("{}Get abstract type", commonIndentString) << endl;
  wasm_instructions += WrapperEncodeMovInt64(10, reinterpret_cast<int64_t>(typeEquivalenceMap.data())); // x10=typeEquivalenceMap
  wasm_instructions += encodeMovz(X_REG, 13, sizeof(size_t));                                           // x13=8
  wasm_instructions += encodeMul(X_REG, 13, 12, 13);                                                    // x13=w12*8
  wasm_instructions += encodeLoadStoreReg(W_REG, LDR, 12, 10, 13); // w12=[x10,x13]=typeEquivalenceMap[actual_type]

  cout << format("{}Get expected abstract type", commonIndentString) << endl;
  wasm_instructions += WrapperEncodeMovInt64(10, reinterpret_cast<int64_t>(typeEquivalenceMap.data())); // x10=typeEquivalenceMap
  wasm_instructions += encodeMovz(X_REG, 13, sizeof(size_t));                                           // x13=8
  wasm_instructions += encodeMovz(W_REG, 14, type_index);                                               // w14=type_index
  wasm_instructions += encodeMul(X_REG, 13, 14, 13);                                                    // x13=w14*8
  wasm_instructions += encodeLoadStoreReg(W_REG, LDR, 14, 10, 13); // w14=[x10,x13]=typeEquivalenceMap[expected_type]

  cout << format("{}Compare types", commonIndentString) << endl;
  wasm_instructions += encodeCompareShift(W_REG, 12, 14);

  cout << format("{}if type doesn't match, goes to longjmp", commonIndentString) << endl;
  fakeInsertBranch("preparelongjmp", "bne"); // if type doesn't match, goes to longjmp

  // step 3: Load parameters
  cout << commonIndentString + "Loading parameters to register before calling" << endl;
  const WasmFunctionType v = wasmFunctionTypeVec[type_index];
  for (int i = v.param_data.size() - 1; i >= 0; --i) {
    wasm_type t = v.param_data[i]; // popping reversely
    switch (getWasmType(t)) {
    case X_REG:
      wasm_instructions += pop(X_REG, false, i);
      break;
    case W_REG:
      wasm_instructions += pop(W_REG, false, i);
      break;
    default:
      throw "Float type unsupported yet";
      break;
    }
  }

  // step 4 & 5: load function address and call
  cout << format("{}Calling indirectly now", commonIndentString) << endl;
  // clear x12
  wasm_instructions += WrapperEncodeMovInt64(12, 0);
  // get function address first
  wasm_instructions += WrapperEncodeMovInt64(12, reinterpret_cast<uint64_t>(in_assembly_call_table));
  wasm_instructions += encodeMovz(X_REG, 13, 8); // x13 = 8
  cout << format("{}Restore x11", commonIndentString) << endl;
  wasm_instructions += encodeMovRegister(X_REG, 11, 16);
  wasm_instructions += encodeMul(X_REG, 11, 11, 13);                                     // x11 = x11 * x13 = x11 * 8
  wasm_instructions += encodeLoadStoreReg(X_REG, LDR, called_function_register, 12, 11); // called_function_register = table[i]
  wasm_instructions += encodeBranchRegister(called_function_register, true);             // blr call_reg
}
void WasmFunction::emitLoop(int i) {
  vector<wasm_type> signature = getSignature(code_vec[i + 1]); // should be all zeros with different types
  string label = "Loop #" + to_string(loop_label++);
  insertLabel(label);
  control_flow_stack.push_back(controlFlowElement(label, signature));
}
void WasmFunction::emitBr(int i) {
  int depth = static_cast<int32_t>(stoul(code_vec[i + 1], nullptr, 16));
  cout << format("Br {}", depth) << endl;
  auto [label, signature] = control_flow_stack[control_flow_stack.size() - depth - 1];
  fakeInsertBranch(label, "b");
};
void WasmFunction::emitBr_if(int i) {
  int depth = static_cast<int32_t>(stoul(code_vec[i + 1], nullptr, 16));
  cout << format("Br_if {}", depth) << endl;
  auto [label, signature] = control_flow_stack[control_flow_stack.size() - depth - 1];
  wasm_instructions += pop(W_REG);
  wasm_instructions += encodeCompareImm(W_REG, 11, 0);
  fakeInsertBranch(label, "bne"); // if true, branches according to label, otherwise falls through
};

void WasmFunction::emitIfOp(int i) {
  /**
  https://github.com/sunfishcode/wasm-reference-manual/blob/master/WebAssembly.md#type-encoding-type
  */
  cout << "If" << endl;
  vector<wasm_type> signature = getSignature(code_vec[i]); // should be all zeros with different types
  string label = "Else/End #" + to_string(if_label++);
  control_flow_stack.push_back(controlFlowElement(label, signature));
  // condition is defined to be i32 type, so going with W_REG here
  wasm_instructions += pop(W_REG);
  wasm_instructions += encodeCompareImm(W_REG, 11, 0);
  fakeInsertBranch(label, "beq"); // if =0, jump to else or end; else continue
  cout << "If true:" << endl;
}
void WasmFunction::emitElseOp() {
  auto [label, signature] = control_flow_stack.back();
  control_flow_stack.pop_back();
  string else_label = "End #" + to_string(if_label++);
  control_flow_stack.push_back(controlFlowElement(else_label, signature));
  fakeInsertBranch(else_label, "b"); // this branch is for previous if end, should't continue executing else instructions, so jmp to end directly
  insertLabel(label);
}
void WasmFunction::emitEndOp() {
  auto [label, signature] = control_flow_stack.back();
  control_flow_stack.pop_back();
  // need to determine if it is bound before inserting
  if (!label_map.contains(label)) {
    insertLabel(label);
  }
}
void WasmFunction::emitReturnOp() {
  cout << "Return" << endl;
  auto [label, signature] = control_flow_stack[0];
  fakeInsertBranch(label, "b");
}
// Count trailing zeros; when all bits are zero, return number of bits, i.e. 32/64
void WasmFunction::emitCtz(RegType regType) {
  /**
   * We need to check if input is 0, and if it is, we return 32/64
   * Otherwise, use rbit and clz to emulate ctz
   */
  cout << format("{}.ctz", regType == X_REG ? "i64" : "i32") << endl;
  // pop one element from stack to r11
  wasm_instructions += pop(regType);
  // moving 32/64 (depending on regType) to x1/w1
  wasm_instructions += encodeMovz(regType, 1, (regType == X_REG) ? 64 : 32);
  // store reversed r11 to r2
  wasm_instructions += encodeRBIT(regType, 2, 11);
  // count r2 leading zeros
  wasm_instructions += encodeCLZ(regType, 2, 2);
  // compare r11 with 0
  wasm_instructions += encodeCompareImm(regType, 11, 0);
  // csel r11, r1, r2, eq
  wasm_instructions += encodeCSEL(regType, 11, 1, 2, reverse_cond_str_map.at("eq"));
  // push r11 to stack
  wasm_instructions += push(regType);
}
// The eqz instruction returns true if the operand is equal to zero, or false otherwise.
// Signature: (i32/i64) : (i32)
void WasmFunction::emitEqz(RegType regType) {
  cout << format("{}.eqz", regType == X_REG ? "i64" : "i32") << endl;
  wasm_instructions += pop(regType);
  // there is no direct equivalent instruction in arm64
  // so we do this by first compare with 0, then use cset to set result
  wasm_instructions += encodeCompareImm(regType, 11, 0);
  wasm_instructions += encodeCSET(W_REG, 11, reverse_cond_str_map.at("ne"));
  wasm_instructions += push(W_REG);
}
// Drop one element from stack
void WasmFunction::emitDrop() {
  cout << "Drop" << endl;
  // decrease reg_pointer_wasm_stack
  wasm_instructions += encodeAddSubImm(X_REG, true, reg_pointer_wasm_stack, reg_pointer_wasm_stack, 8);
}
void WasmFunction::constructFullinstr(string sub_instr) {
  wasm_instructions = wasm_instructions + sub_instr;
}
