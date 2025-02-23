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
void WasmFunction::push(wasm_type val) {
  /**
   * There are mainly two things that needs to be done here
   * Store the value to the wasm stack, update its pointer
   * Update the type size stack and its pointer
   * We wrap them up in this function to ensure that they are done together
   */
  auto regType = getWasmType(val);
  std::visit(
      [this, regType](auto &&value) {
        // load value to r11
        wasm_instructions += encodeMovz(11, value, regType);
        // store value in r11 to wasm_stack[REG_POINTER_WASM_STACK]
        wasm_instructions += encodeLoadStoreReg(regType, STR, 11, REG_WASM_STACK, REG_POINTER_WASM_STACK);
        // add REG_POINTER_WASM_STACK by 8
        wasm_instructions += encodeAddSubImm(X_REG, false, REG_POINTER_WASM_STACK, REG_POINTER_WASM_STACK, 8);
        // load size info to register
        if (regType == W_REG || regType == D_REG) {
          wasm_instructions += encodeMovz(11, 4, X_REG);
        } else {
          wasm_instructions += encodeMovz(11, 8, X_REG);
        }
        // store size info in type_stack
        wasm_instructions += encodeLoadStoreReg(X_REG, STR, 11, REG_TYPE_SIZE_STACK, REG_POINTER_TYPE_SIZE);
        // add REG_POINTER_TYPE_SIZE by 8
        wasm_instructions += encodeAddSubImm(X_REG, false, REG_POINTER_TYPE_SIZE, REG_POINTER_TYPE_SIZE, 8);
      },
      val);
}
void WasmFunction::pop() {
  /*
   * read type from type_stack
   * read from wasm_stack using type
   * update pointer
   * NOTE: Pop result will be stored in r11
   *
   * HINT: lldb read memory usage
   * read 4 bytes from [x20, x22]
   * memory read -f x -c 4 `$x20 + $x22`
   */
  // decrease REG_POINTER_TYPE_SIZE
  wasm_instructions += encodeAddSubImm(X_REG, true, REG_POINTER_TYPE_SIZE, REG_POINTER_TYPE_SIZE, 8);
  // read from type_stack
  // can't just use REG_POINTER_TYPE_SIZE, need to read the value in it
  wasm_instructions += encodeLoadStoreReg(X_REG, LDR, 11, REG_TYPE_SIZE_STACK, REG_POINTER_TYPE_SIZE);
  // decrease REG_POINTER_WASM_STACK
  wasm_instructions += encodeAddSubImm(X_REG, true, REG_POINTER_WASM_STACK, REG_POINTER_WASM_STACK, 8);
  // compare r11 with 4/8
  wasm_instructions += encodeCompareImm(X_REG, 11, 4);
  wasm_instructions += encodeBranchCondition(3, reverse_cond_str_map.at("ne")); // if size!= 4, goto 2 ahead
  // TODO: only support W_REG(4) and X_REG(8) here
  wasm_instructions += encodeLoadStoreReg(W_REG, LDR, 11, REG_WASM_STACK, REG_POINTER_WASM_STACK); // this means size == 4
  wasm_instructions += encodeBranch(2);
  wasm_instructions += encodeLoadStoreReg(X_REG, LDR, 11, REG_WASM_STACK, REG_POINTER_WASM_STACK);
}