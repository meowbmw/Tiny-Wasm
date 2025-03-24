#pragma once
#include "WasmFunction.hpp"

void *get_memcpy_address() {
  return dlsym(RTLD_DEFAULT, "memcpy");
}

void *get_printf_address() {
  return dlsym(RTLD_DEFAULT, "printf");
}

/**
 *
 * Allocate memory with execute permission
 * And load machine code into that
 * Return a function pointer to the allocated address
 *
 */
template <typename Func> auto getFunctionPointer(string full_instructions) -> Func {
  const size_t arraySize = full_instructions.length() / 2;
  // 使用静态变量保存地址，确保每次调用使用相同地址
  static void *last_addr = nullptr;
  // 如果已有地址，先释放
  if (last_addr != nullptr) {
    munmap(last_addr, arraySize);
  }
  void *ptr = mmap(last_addr, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  // 保存新地址
  last_addr = ptr;
  char *functionAddr = reinterpret_cast<char *>(ptr);
  for (size_t i = 0; i < arraySize; ++i) {
    const string byteStr = full_instructions.substr(i * 2, 2);
    functionAddr[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
  }
  __builtin___clear_cache(functionAddr, functionAddr + arraySize);
  return reinterpret_cast<Func>(functionAddr);
}
int64_t WasmFunction::executeWasmInstr() {
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
  auto instruction_set = getFunctionPointer<int64_t (*)(void *, char *, void *, void *, void *, void *, int)>(full_instructions);
  void *buffer = calloc(2048, sizeof(int)); // use calloc to initialize memory to 0, to avoid garbage data
  void *memcpy_location = get_memcpy_address();
  void *printf_location = get_printf_address();
  int max_page = (VecMemInfo.size() > 0) ? VecMemInfo[0].max_page : 0;
  // !不需要做任何传参，因为参数已经放在寄存器里啦
  int64_t ans = instruction_set(buffer, globalVars, memorySizeKeeper, memoryInitializeFunction, memcpy_location, printf_location, max_page);
  auto return_code = *reinterpret_cast<int16_t *>(buffer);
  cout << "Return code is: " << return_code << endl; // anything other than 0 means exception raised!
  free(buffer);
  // munmap(reinterpret_cast<void *>(instruction_set), full_instructions.length() / 2); // GC here
  // WARN: reset things, very important if we want to call it again!
  clear();
  if (return_code) {
    throw string("Wasm trapped");
  }
  return ans;
}
