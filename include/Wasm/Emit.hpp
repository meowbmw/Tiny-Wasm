#pragma once
#include "WasmFunction.hpp"

void WasmFunction::emitGet(const uint64_t var_to_get, TypeCategory vecType) {
  /**
   * Local.get i
   * push to wasm stack memory[var[i]]
   * var[i] -> x/w11 -> stack[top]
   */

  // todo: probably problematic after introducing if-else structure
  // may need a major overhaul
  RegType regtype = regTypeGetter[{vecType, var_to_get}];
  int stack_offset = vecToStack[{vecType, var_to_get}];
  cout << format("Getting {}[{}]", type_category_to_string(vecType), var_to_get) << endl;
  // Note: We use x11 as a bridge register for memory -> memory transfer!
  string load_param_instr = encodeLoadStoreImm(regtype, LDR, 11, 31, stack_offset);
  // var[i] -> x/w11
  string store_to_stack_instr = encodeLoadStoreImm(regtype, STR, 11, 31, wasm_stack_pointer);
  // x/w11 -> stack[top]
  wasm_stack_pointer -= 8; // decrease wasm stack after push
  constructFullinstr(load_param_instr + store_to_stack_instr);
}
void WasmFunction::emitSet(const uint64_t var_to_set, TypeCategory vecType, bool isTee) {
  /**
   * Local.set 0
   * Set memory[var[i]] to top value of wasm stack
   * stack[top] -> x/w11 -> var[i]
   */
  RegType regtype = regTypeGetter[{vecType, var_to_set}];
  int stack_offset = vecToStack[{vecType, var_to_set}];
  cout << format("Assigning to {}[{}]", type_category_to_string(vecType), var_to_set) << endl;
  wasm_stack_pointer += 8;
  string store_to_stack_instr = encodeLoadStoreImm(regtype, LDR, 11, 31, wasm_stack_pointer);
  string reg_to_mem_instr = encodeLoadStoreImm(regtype, STR, 11, 31, stack_offset);
  if (isTee) {
    wasm_stack_pointer -= 8; // teeing keeps stack intact
  }
  constructFullinstr(store_to_stack_instr + reg_to_mem_instr);
}
void WasmFunction::emitConst(wasm_type elem) {
  /***
   * push value $elem onto wasm Stack
   * mov $elem, x11
   * str x11, [sp+wasm_stack_pointer]
   */
  std::visit(
      [this](auto &&value) {
        char typeInfo = typeid(value).name()[0];
        if (typeInfo == 'f') {
          // todo: don't support fmov yet
          throw std::invalid_argument("Don't support float const yet; need to properly implement fmov or store float first");
          // string load_to_reg_instr=toHexString(encodeFmovz()).substr(2);
          // string store_to_stack_instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_F32, 11, 31, wasm_stack_pointer, false)).substr(2);
        } else if (typeInfo == 'd') {
          throw std::invalid_argument("Don't support double const yet; need to properly implement fmov or store double first");
        } else if (typeInfo == 'i') {
          cout << format("i32.const {}", value) << endl;
          string load_to_reg_instr = WrapperEncodeMovInt32(11, value);
          // cout << format("Emit: mov {}, w11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
          string store_to_stack_instr = encodeLoadStoreImm(W_REG, STR, 11, 31, wasm_stack_pointer);
          constructFullinstr(load_to_reg_instr + store_to_stack_instr);
        } else if (typeInfo == 'l') {
          cout << format("i64.const {}", value) << endl;
          string load_to_reg_instr = WrapperEncodeMovInt64(11, value);
          // cout << format("Emit: mov {}, x11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
          string store_to_stack_instr = encodeLoadStoreImm(X_REG, STR, 11, 31, wasm_stack_pointer);
          constructFullinstr(load_to_reg_instr + store_to_stack_instr);
        }
      },
      elem);
  wasm_stack_pointer -= 8;
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
  wasm_stack_pointer += 8;
  // r11 = b
  string load_second_param_instr = encodeLoadStoreImm(regtype, LDR, 11, 31, wasm_stack_pointer);
  wasm_stack_pointer += 8;
  // r12 = a
  string load_first_param_instr = encodeLoadStoreImm(regtype, LDR, 12, 31, wasm_stack_pointer);

  constructFullinstr(load_first_param_instr + load_second_param_instr);
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
  wasm_instructions += encodeLoadStoreImm(regtype, STR, 11, 31, wasm_stack_pointer); // store_to_stack
  wasm_stack_pointer -= 8;                                                           // decrease wasm stack after push
}
void WasmFunction::emitIfOp(int i) {
  /**
  https://github.com/sunfishcode/wasm-reference-manual/blob/master/WebAssembly.md#type-encoding-type
  */
  vector<wasm_type> signature; // should be all zeros with different types
  if (code_vec[i] == "7f") {   // i32
    signature.push_back(static_cast<int32_t>(0));
  } else if (code_vec[i] == "7e") { // i64
    signature.push_back(static_cast<int64_t>(0));
  } else if (code_vec[i] == "7d") { // f32
    throw "return f32 after if not implemented yet";
  } else if (code_vec[i] == "7c") { // f64
    throw "return f64 after if not implemented yet";
  } else if (code_vec[i] == "70") { // funcref
    throw "return funcref after if not implemented yet";
  } else if (code_vec[i] == "60") { // func
    throw "return func after if not implemented yet";
  } else if (code_vec[i] == "40") { // void
    // no need to push anything
  } else {
    throw format("invalid byte after if: {}. Check wasm binary integrity", code_vec[i]);
  }
  string label = "Else/End_" + to_string(if_label++);
  control_flow_stack.push_back(controlFlowElement(label, stack.size(), signature));
  RegType regtype = getWasmType(stack.back());
  cout << "Compare for if:" << endl;
  wasm_instructions += encodeLoadStoreImm(regtype, LDR, 11, 31, wasm_stack_pointer + 8);
  wasm_instructions += encodeCompareImm(regtype, 11, 0);
  fakeInsertBranch(label, "beq"); // if =0, jump to else or end; else continue
  cout << "If true:" << endl;
}
void WasmFunction::emitElseOp() {
  auto [label, stack_length, signature] = control_flow_stack.back();
  control_flow_stack.pop_back();
  string else_label = "End_" + to_string(if_label++);
  control_flow_stack.push_back(controlFlowElement(else_label, stack.size(), signature));
  fakeInsertBranch(else_label, "b"); // this branch is for previous if end, should't continue executing else instructions, so jmp to end directly
  insertLabel(label);
}
void WasmFunction::emitEndOp() {
  auto [label, stack_length, signature] = control_flow_stack.back();
  control_flow_stack.pop_back();
  insertLabel(label);
}
void WasmFunction::emitRet() {
  string instr = encodeReturn();
  constructFullinstr(instr);
}

void WasmFunction::constructFullinstr(string sub_instr) {
    wasm_instructions = wasm_instructions + sub_instr;
  }
  
