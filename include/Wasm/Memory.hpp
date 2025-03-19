#pragma once
#include "WasmFunction.hpp"

// use mmap to allocateMemory
// Usage: mmap (void *__addr, size_t __len, int __prot,
//    int __flags, int __filedescripter, __off_t __offset);
string allocateMemory() {
  string instr;
  instr += encodeMovz(X_REG, 0, 0); // x0 = addr (usually kernel will provide this)
  instr += encodeMovz(X_REG, 2, PROT_READ | PROT_WRITE);
  instr += encodeMovz(X_REG, 3, MAP_PRIVATE | MAP_ANONYMOUS);
  instr += encodeMovn(X_REG, 4, 0, 0); // x4 = -1, we use movn x4, 0 to load -1
  instr += encodeMovz(X_REG, 5, 0);

  instr += encodeMovz(X_REG, 8, __NR_mmap); // x8 is syscall number

  instr += encodeSVC(0); // invoke system call
  instr +=
      encodeNop(); // todo: for some reason, the instruction immediately followed svc will be skipped, so we use nop here for that skipped instruction
  return instr;
}
string allocateMemory(uint16_t page_size) {
  cout << format("Allocating Memory with page size: {}", page_size) << endl;
  string instr;
  instr += WrapperEncodeMovInt64(1, page_size * 65536); // x1 = allocate size
  instr += allocateMemory();
  cout << "Result address will be moved to reg_pointer_wasm_memory" << endl;
  return instr;
}

// use munmap to free memory
// Usage: int munmap (void *__addr, size_t __len);
string releaseMemory() {
  string instr;
  instr += encodeMovz(X_REG, 8, __NR_munmap); // x8 is syscall number(215 is munmap)
  instr += encodeSVC(0);                      // invoke system call
  instr +=
      encodeNop(); // todo: for some reason, the instruction immediately followed svc will be skipped, so we use nop here for that skipped instruction

  return instr;
}
string releaseMemory(uint16_t page_size) {
  string instr;
  instr += WrapperEncodeMovInt64(1, page_size * 65536); // x1 = size to release
  instr += releaseMemory();
  return instr;
}

string WasmFunction::growMemory() {
  // 1. allocate new memory
  // 2. copy from old memory
  // 3. release old memory
  string instr;
  // 1. allocate new memory
  // new page size is already saved to x1
  cout << format("{}Step 1. Allocating new memory", commonIndentString) << endl;
  instr += encodeMovRegister(X_REG, 11, 1); // backup new page size to x11
  instr += allocateMemory();                // set copy dest to x0
  // void *memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n);
  // 2. copy from old memory
  // new memory dest is already in x0
  cout << format("{}Step 2. Copying from old memory", commonIndentString) << endl;
  instr += encodeMovRegister(X_REG, 1, reg_pointer_wasm_memory);  // set copy src to x1
  instr += encodeLoadStoreImm(W_REG, LDR, 2, reg_memory_size, 0); // load old memory size to x2
  instr += encodeMovRegister(X_REG, 14, 30);                      // backup x30 before blr
  instr += encodeBranchRegister(25, true);
  instr += encodeMovRegister(X_REG, 30, 14);                     // restore x30
  instr += encodeMovRegister(X_REG, reg_pointer_wasm_memory, 0); // set mem pointer by x0
  // 3. release old memory
  cout << format("{}Step 3. Release old memory", commonIndentString) << endl;
  instr += encodeMovRegister(X_REG, 0, 1);                         // set x0=old memory pointer
  instr += encodeLoadStoreImm(W_REG, LDR, 1, reg_memory_size, 0);  // set x1=old page size
  instr += push(W_REG, 1);                                         // push old page size
  instr += releaseMemory();                                        // release old memory
  instr += encodeLSRImm(W_REG, 11, 11, 16);                        // divide by 65536
  instr += encodeLoadStoreImm(W_REG, STR, 11, reg_memory_size, 0); // update memory size;
  return instr;
}