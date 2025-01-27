#pragma once
#include "WasmFunction.hpp"

template <typename Func> auto WasmFunction::getFunctionPointer(string full_instructions) -> Func {
  const size_t arraySize = full_instructions.length() / 2;
  auto charArray = make_unique<unsigned char[]>(arraySize); // 使用智能指针
  for (size_t i = 0; i < arraySize; ++i) {
    const string byteStr = full_instructions.substr(i * 2, 2);
    charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
  }
  Func instruction_set = nullptr;
  instruction_set = reinterpret_cast<Func>(mmap(nullptr, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (instruction_set == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  memcpy(reinterpret_cast<void *>(instruction_set), charArray.get(), arraySize);
  __builtin___clear_cache(reinterpret_cast<char *>(instruction_set), reinterpret_cast<char *>(instruction_set) + arraySize);
  return instruction_set;
}
int64_t WasmFunction::executeWasmInstr() {
  /**
   *
   * Allocate memory with execute permission
   * And load machine code into that
   *
   */
  // Warn: Append pre wasm_instructions here
  // string full_instructions = pre_instructions_for_param_loading + encodeBranch(1) + wasm_instructions;
  string full_instructions = pre_instructions_for_param_loading + wasm_instructions;
  cout << "Machine instruction to load: " << full_instructions << endl;
  if (pre_instructions_for_param_loading.size() > 0) {
    cout << " - Load param instr: ";
    for (size_t i = 0; i < pre_instructions_for_param_loading.size(); i += 8) {
      if (i > 0) {
        std::cout << " | ";
      }
      std::cout << pre_instructions_for_param_loading.substr(i, 8);
    }
    cout << endl;
  }
  if (wasm_instructions.size() > 0) {
    cout << " - Run wasm instr: ";
    for (size_t i = 0; i < wasm_instructions.size(); i += 8) {
      if (i > 0) {
        std::cout << " | ";
      }
      std::cout << wasm_instructions.substr(i, 8);
    }
    cout << endl;
  }
  auto instruction_set = getFunctionPointer<int64_t (*)(void *)>(full_instructions);
  void *buffer = malloc(1024);
  // !不需要做任何传参，因为参数已经放在寄存器里啦
  int64_t ans = instruction_set(buffer);
  auto return_code = *reinterpret_cast<int16_t *>(buffer);
  cout << "Return code is: " << return_code << endl;
  free(buffer);
  munmap(reinterpret_cast<void *>(instruction_set), full_instructions.size()); // GC here
  // WARN: reset things, very important if we want to call it again!
  resetAfterExecution();
  if (return_code) {
    throw string("Wasm trapped");
  }
  return ans;
}
