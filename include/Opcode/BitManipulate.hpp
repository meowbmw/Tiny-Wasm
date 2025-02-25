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