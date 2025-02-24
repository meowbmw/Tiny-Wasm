#pragma once
#include "WasmFunction.hpp"

void WasmFunction::getStackPreallocateSize(const int offset) {
  /**
   * calculate how much size should be allocated for stack
   */
  streambuf *old = cout.rdbuf();
  cout.rdbuf(0);
  param_stack_start_location = 0;
  param_stack_end_location = param_stack_start_location;
  for (const auto &c : param_data) {
    allocateVar(c, param_stack_end_location);
  }
  local_stack_start_location = param_stack_end_location + 8;
  local_stack_end_location = local_stack_start_location;
  for (const auto &c : local_data) {
    allocateVar(c, local_stack_end_location);
  }
  cout << "--- Estimate stack allocation ---" << endl;
  cout << "Param start location: " << param_stack_start_location << endl;
  cout << "Param end location: " << param_stack_end_location << endl;
  cout << "Local start location: " << local_stack_start_location << endl;
  cout << "Local end location: " << local_stack_end_location << endl;
  // align to neaest 16 byte
  if (local_stack_end_location % 16 != 0) {
    stack_size = 16 * (local_stack_end_location / 16 + 1);
  } else {
    stack_size = local_stack_end_location;
  }
  cout << "Stack allocate size estimated to be: " << stack_size << endl;
  cout.rdbuf(old);
}
void WasmFunction::prepareSp() {
  cout << "Sub sp register" << endl;
  string instr = encodeAddSubImm(X_REG, true, 31, 31, stack_size); // sub sp, sp, stack_size
  constructFullinstr(instr);
}
void WasmFunction::printInitStack() {
  cout << "--- Printing initial stack ---" << endl;
  for (const auto &p : stackToVec) {
    cout << format("[sp, #0x{:x}] = {}[{}]", p.first, type_category_to_string(p.second.first), p.second.second) << endl;
  }
}
void WasmFunction::restoreSP() {
  // getting result and restoring sp register
  cout << "Moving stack top to register as result" << endl;
  string prepare_ans_instr;
  for (int i = 0; i < result_data.size(); ++i) {
    // todo: we should be iterating here; i < result.size()
    // but we are actually expecting i=0 only (1 result)
    std::visit(
        [&i, &prepare_ans_instr, this](auto &&value) {
          char typeInfo = typeid(value).name()[0];
          if (typeInfo == 'f') {
            throw std::invalid_argument("Fmov not supported yet!");
          } else if (typeInfo == 'd') {
            throw std::invalid_argument("Fmov not supported yet!");
          } else if (typeInfo == 'l') {
            prepare_ans_instr += pop(X_REG, false, i);
          } else if (typeInfo == 'i') {
            prepare_ans_instr += pop(W_REG, false, i);
          }
        },
        result_data[i]);
  }
  cout << "Restore sp register" << endl;
  const string restore_sp_instr = encodeAddSubImm(X_REG, false, 31, 31, stack_size); // add sp, sp, stack_size
  constructFullinstr(prepare_ans_instr + restore_sp_instr);
}
string WasmFunction::push(RegType regType, int save_reg) {
  /**
   * Warn: We assume that value is already stored in save_reg (default = 11)
   * This function does:
   * (1) Save value to stack
   * (2) Update its stack pointer
   */
  cout << "***Push stack***" << endl;
  string instr;
  // store value in save_reg to wasm_stack[REG_POINTER_WASM_STACK]
  instr += encodeLoadStoreReg(regType, STR, save_reg, REG_WASM_STACK, REG_POINTER_WASM_STACK);
  // add REG_POINTER_WASM_STACK by 8
  instr += encodeAddSubImm(X_REG, false, REG_POINTER_WASM_STACK, REG_POINTER_WASM_STACK, 8);
  cout << "***Push stack End***" << endl;
  return instr;
}
string WasmFunction::pop(RegType regType, bool tee = false, int save_reg) {
  /*
   * NOTE: result will be stored in save_reg (default = 11)
   * This function does:
   * (1) update pointer
   * (2) read from wasm_stack
   * (3) (Optional) if teeing, restore pointer
   */

  cout << format("***{} stack***", tee == true ? "Tee" : "Pop") << endl;
  string instr;
  // decrease REG_POINTER_WASM_STACK
  instr += encodeAddSubImm(X_REG, true, REG_POINTER_WASM_STACK, REG_POINTER_WASM_STACK, 8);
  instr += encodeLoadStoreReg(regType, LDR, save_reg, REG_WASM_STACK, REG_POINTER_WASM_STACK);
  if (tee) {
    cout << "Teeing so restoring stack pointers!" << endl;
    instr += encodeAddSubImm(X_REG, false, REG_POINTER_WASM_STACK, REG_POINTER_WASM_STACK, 8);
  }
  cout << format("***{} stack end***", tee == true ? "Tee" : "Pop") << endl;
  cout << "***Pop stack end***" << endl;
  return instr;
}