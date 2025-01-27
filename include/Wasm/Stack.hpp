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
  wasm_stack_start_location = local_stack_end_location + 8;
  int wasm_stack_size = (code_vec.size() - offset) * 4;
  wasm_stack_end_location = wasm_stack_start_location + wasm_stack_size;
  cout << "Wasm stack start location: " << wasm_stack_start_location << endl;
  cout << format("Adding maximum possible wasm stack size: (code_vec.size: {} - offset: {}) * 4 = {}", code_vec.size(), offset, wasm_stack_size)
       << endl;
  cout << "Wasm stack end location: " << wasm_stack_end_location << endl;
  // align to neaest 16 byte
  if (wasm_stack_end_location % 16 != 0) {
    stack_size = 16 * (wasm_stack_end_location / 16 + 1);
  } else {
    stack_size = wasm_stack_end_location;
  }
  cout << "Stack allocate size estimated to be: " << stack_size << endl;
  cout.rdbuf(old);
}
void WasmFunction::prepareStack() {
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
void WasmFunction::restoreStack() {
  // getting result and restoring sp register
  cout << "Moving stack top to register as result" << endl;
  string prepare_ans_instr;
  int current_wasm_pointer = wasm_stack_pointer + 8;
  for (int i = 0; i < result_data.size(); ++i) {
    // todo: we should be iterating here; i < result.size()
    // but we are actually expecting i=0 only (1 result)
    std::visit(
        [&i, &prepare_ans_instr, &current_wasm_pointer, this](auto &&value) {
          char typeInfo = typeid(value).name()[0];
          if (typeInfo == 'f') {
            throw std::invalid_argument("Fmov not supported yet!");
          } else if (typeInfo == 'd') {
            throw std::invalid_argument("Fmov not supported yet!");
          } else if (typeInfo == 'l') {
            prepare_ans_instr += encodeLoadStoreImm(X_REG, LDR, i, 31, current_wasm_pointer);
          } else if (typeInfo == 'i') {
            prepare_ans_instr += encodeLoadStoreImm(W_REG, LDR, i, 31, current_wasm_pointer);
          }
        },
        result_data[i]);
    current_wasm_pointer -= 8;
  }
  cout << "Restore sp register" << endl;
  const string restore_sp_instr = encodeAddSubImm(X_REG, false, 31, 31, stack_size); // add sp, sp, stack_size
  constructFullinstr(prepare_ans_instr + restore_sp_instr);
}
void WasmFunction::commonStackOp(char opType) {
  auto b = stack.back();
  stack.pop_back();
  auto a = stack.back();
  stack.pop_back();
  stack.push_back(operations_map[opType](a, b));
}
