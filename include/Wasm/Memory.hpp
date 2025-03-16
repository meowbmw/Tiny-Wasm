#pragma once
#include "WasmFunction.hpp"

// use mmap to allocateMemory
// Usage: mmap (void *__addr, size_t __len, int __prot,
//    int __flags, int __filedescripter, __off_t __offset);
string allocateMemory(uint16_t page_size) {
  cout << format("Allocating Memory with page size: {}", page_size) << endl;
  string instr;
  instr += encodeMovz(X_REG, 0, 0);                     // x0 = addr (usually kernel will provide this)
  instr += WrapperEncodeMovInt64(1, page_size * 65536); // x1 = allocate size
  instr += encodeMovz(X_REG, 2, PROT_READ | PROT_WRITE);
  instr += encodeMovz(X_REG, 3, MAP_PRIVATE | MAP_ANONYMOUS);
  instr += encodeMovn(X_REG, 4, 0, 0); // x4 = -1, we use movn x4, 0 to load -1
  instr += encodeMovz(X_REG, 5, 0);

  instr += encodeMovz(X_REG, 8, __NR_mmap); // x8 is syscall number

  instr += encodeSVC(0); // invoke system call
  instr +=
      encodeNop(); // todo: for some reason, the instruction immediately followed svc will be skipped, so we use nop here for that skipped instruction
  cout << "Result address will be moved to reg_pointer_wasm_memory" << endl;
  return instr;
}

// use munmap to free memory
// Usage: int munmap (void *__addr, size_t __len);
string releaseMemory(uint16_t page_size) {
  string instr;
  instr += WrapperEncodeMovInt64(1, page_size * 65536); // x1 = size to release
  instr += encodeMovz(X_REG, 8, __NR_munmap);           // x8 is syscall number(215 is munmap)
  instr += encodeSVC(0);                                // invoke system call
  instr +=
      encodeNop(); // todo: for some reason, the instruction immediately followed svc will be skipped, so we use nop here for that skipped instruction

  return instr;
}

string growMemory() {
  // 1. allocate new memory
  // 2. copy from old memory
  // 3. release old memory
  string instr;
  // 1. allocate new memory
  instr += encodeMovz(X_REG, 0, 0);                     // x0 = addr (usually kernel will provide this)
  // instr += WrapperEncodeMovInt64(1, page_size * 65536); // x1 = allocate size
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