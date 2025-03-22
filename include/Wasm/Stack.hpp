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
  stack_size += 16; // allocate additional space for backup x30
  cout << "Stack allocate size estimated to be: " << stack_size << endl;
  cout.rdbuf(old);
}
void WasmFunction::prepareSp() {
  cout << commonIndentString + "-Prepare function entering" << endl;
  string instr = encodeLdpStp(X_REG, STR, 29, 30, 31, -0x20, EncodingMode::PreIndex); // stp x29, x30, [sp, #-0x20]!
  instr += encodeMovSP(X_REG, 29, 31);                                                      // x29 = sp
  instr += encodeAddSubImm(X_REG, true, 31, 31, stack_size);                                // sub sp, sp, stack_size
  prep_sp_instr = instr;
  constructFullinstr(instr);
}
void WasmFunction::printInitStack() {
  cout << "--- Printing initial stack ---" << endl;
  for (const auto &p : stackToVec) {
    cout << format("[sp, #0x{:x}] = {}[{}]", p.first, type_category_to_string(p.second.first), p.second.second) << endl;
  }
}
void WasmFunction::getResult() {
  // Moving stack top to register as result
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
  constructFullinstr(prepare_ans_instr);
}
void WasmFunction::restoreSP() {
  cout << "Prepare function return" << endl;
  string restore_sp_instr = encodeAddSubImm(X_REG, false, 31, 31, stack_size);                   // add sp, sp, stack_size
  restore_sp_instr += encodeLdpStp(X_REG, LDR, 29, 30, 31, 0x20, EncodingMode::PostIndex); // ldp	x29, x30, [sp], #16
  this->restore_sp_instr = restore_sp_instr;
  constructFullinstr(restore_sp_instr);
}
string WasmFunction::push(RegType regType, int save_reg) {
  /**
   * Warn: We assume that value is already stored in save_reg (default = 11)
   * This function does:
   * (1) Save value to stack
   * (2) Update its stack pointer
   */
  cout << commonIndentString + "+Push stack" << endl;
  string instr;
  // store value in save_reg to wasm_stack[reg_pointer_wasm_stack]
  instr += encodeLoadStoreReg(regType, STR, save_reg, reg_wasm_stack, reg_pointer_wasm_stack);
  // add reg_pointer_wasm_stack by 8
  instr += encodeAddSubImm(X_REG, false, reg_pointer_wasm_stack, reg_pointer_wasm_stack, 8);
  cout << commonIndentString + "+Push stack End" << endl;
  return instr;
}
string WasmFunction::pop(RegType regType, bool tee, int save_reg) {
  /*
   * NOTE: result will be stored in save_reg (default = 11)
   * This function does:
   * (1) update pointer
   * (2) read from wasm_stack
   * (3) (Optional) if teeing, restore pointer
   */

  cout << format("{}{} stack", commonIndentString, tee == true ? "|Tee" : "-Pop") << endl;
  string instr;
  // decrease reg_pointer_wasm_stack
  instr += encodeAddSubImm(X_REG, true, reg_pointer_wasm_stack, reg_pointer_wasm_stack, 8);
  instr += encodeLoadStoreReg(regType, LDR, save_reg, reg_wasm_stack, reg_pointer_wasm_stack);
  if (tee) {
    cout << commonIndentString + "Teeing so restoring stack pointers!" << endl;
    instr += encodeAddSubImm(X_REG, false, reg_pointer_wasm_stack, reg_pointer_wasm_stack, 8);
  }
  cout << format("{}{} stack end", commonIndentString, tee == true ? "|Tee" : "-Pop") << endl;
  return instr;
}