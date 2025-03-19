#pragma once
#include "Base.hpp"

// Reverses the bit order in a register.
// Note this is not reversing bit by bit! This is reversing the order!
// RBIT <Rd>, <Rn>
string encodeRBIT(RegType regType, uint8_t rd, uint8_t rn, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b101101011, 22);
  opcode.setRd(rd);
  opcode.setRn(rn);
  return opcode.getInstruction();
}

// Count leading zeros
// CLZ <Rd>, <Rn>
string encodeCLZ(RegType regType, uint8_t rd, uint8_t rn, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b101101011, 22);
  opcode.setRd(rd);
  opcode.setRn(rn);
  opcode.setField(1, 12);
  return opcode.getInstruction();
}

string encodeLSRImm(RegType regType, uint8_t rd, uint8_t rn, uint8_t shift, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b101, 28);
  opcode.setField(0b11, 24);
  if (regType == X_REG) {
    opcode.setField(1, 22);
    opcode.setField(1, 15);
  }
  opcode.setField(0b11111, 10);
  uint8_t maxShift = (regType == W_REG) ? 31 : 63;
  if (shift > maxShift) {
    throw std::runtime_error("Shift amount out of range in LSRimm!");
  }
  opcode.setField(shift, 16);
  opcode.setRn(rn);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

/**
 * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/LSL--immediate---Logical-shift-left--immediate---an-alias-of-UBFM-?lang=en
 * This instruction shifts a register value left by an immediate number of bits, shifting in zeros, and writes the result to the destination register.
 */
string encodeLSLImm(RegType regType, uint8_t rd, uint8_t rn, uint8_t shift, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b101, 28);
  opcode.setField(0b11, 24);
  if (regType == X_REG) {
    opcode.setField(1, 22);
    opcode.setField(1, 15);
  }
  uint8_t maxShift = (regType == W_REG) ? 31 : 63;
  if (shift > maxShift) {
    throw std::runtime_error("Shift amount out of range in LSLimm!");
  }
  /*
  LSL（Logical Shift Left）是一个伪指令​（pseudo-instruction），其底层由 UBFM（Unsigned Bitfield Move）指令实现。指令中的
  imms 和 immr 字段是 UBFM 的编码参数 ARM64 的位域操作指令（如 UBFM）通过 imms 和 immr 定义位域的范围：
  ​**immr**：右移位数（对于左移，immr = 64
  - shift）。
  ​**imms**：位域结束位置（对于左移，imms = 63 - shift）。
  */
  if (regType == X_REG) {
    opcode.setField(64 - shift, 16);
    opcode.setField(63 - shift, 10);
  } else {
    opcode.setField(32 - shift, 16);
    opcode.setField(31 - shift, 10);
  }
  opcode.setRn(rn);
  opcode.setRd(rd);
  return opcode.getInstruction();
}