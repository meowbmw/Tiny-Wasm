#pragma once
#include "WasmFunction.hpp"

void WasmFunction::emitGet(const uint64_t var_to_get, TypeCategory vecType) {
  /**
   * Local.get i
   * push to wasm stack memory[var[i]]
   * var[i] -> x/w11 -> stack[top]
   */
  RegType regtype = regTypeGetter[{vecType, var_to_get}];
  int stack_offset = vecToStack[{vecType, var_to_get}];
  cout << format("Getting {}[{}]", type_category_to_string(vecType), var_to_get) << endl;
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
  cout << format("Assigning to {}[{}]", type_category_to_string(vecType), var_to_set) << endl;
  string load_to_reg_instr = pop(regtype, isTee);
  string reg_to_mem_instr = encodeLoadStoreImm(regtype, STR, 11, 31, stack_offset);
  constructFullinstr(load_to_reg_instr + reg_to_mem_instr);
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
    cout << "i64." << condStr << endl;
    break;
  case W_REG:
    cout << "i32." << condStr << endl;
    break;
  default:
    throw "Unsupported reg type (float or double)";
  }
  // todo: this could be simplified by one cset instruction
  wasm_instructions += encodeMovz(3, 1, X_REG); // x3=1
  wasm_instructions += encodeMovz(4, 0, X_REG); // x4=0
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
  control_flow_stack.push_back(controlFlowElement(label, signature));
  cout << "Compare for if:" << endl;
  // condition is defined to be i32 type, so going with W_REG here
  wasm_instructions += pop(W_REG, true);
  wasm_instructions += encodeCompareImm(W_REG, 11, 0);
  fakeInsertBranch(label, "beq"); // if =0, jump to else or end; else continue
  cout << "If true:" << endl;
}
void WasmFunction::emitElseOp() {
  auto [label, signature] = control_flow_stack.back();
  control_flow_stack.pop_back();
  string else_label = "End_" + to_string(if_label++);
  control_flow_stack.push_back(controlFlowElement(else_label, signature));
  fakeInsertBranch(else_label, "b"); // this branch is for previous if end, should't continue executing else instructions, so jmp to end directly
  insertLabel(label);
}
void WasmFunction::emitEndOp() {
  auto [label, signature] = control_flow_stack.back();
  control_flow_stack.pop_back();
  insertLabel(label);
}

// Drop one element from stack
void WasmFunction::emitDrop() {
  cout << "Drop" << endl;
  // decrease REG_POINTER_WASM_STACK
  wasm_instructions += encodeAddSubImm(X_REG, true, REG_POINTER_WASM_STACK, REG_POINTER_WASM_STACK, 8);
}
void WasmFunction::emitRet() {
  string instr = encodeReturn();
  constructFullinstr(instr);
}

void WasmFunction::constructFullinstr(string sub_instr) {
  wasm_instructions = wasm_instructions + sub_instr;
}
