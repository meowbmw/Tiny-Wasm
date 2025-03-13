#pragma once
#include "Base.hpp"

string encodeReturn(uint8_t rn = 30, bool smallEndian = true, bool printFlag = true) {
  auto opcode = Arm64Opcode(smallEndian, printFlag);
  opcode.setField(0b1101011001011111000000, 10);
  opcode.setRn(rn);
  return opcode.getInstruction();
}

string encodeBranch(int32_t imm, bool withLink = false, bool smallEndian = true) {
  // offset is instruction level, i.e. forward 5 instruction => offset = 5
  // this is different from the origin b label!!
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(0b101, 26);
  opcode.setField((int)(withLink), 31);
  opcode.setField(imm, 0, 26);
  return opcode.getInstruction();
}

string encodeBranchCondition(int32_t imm19, uint8_t cond, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(0b01010100, 24);
  opcode.setImm19(imm19);
  opcode.setField(cond, 0, 4);
  return opcode.getInstruction();
}

string encodeBranchRegister(uint8_t rn, bool hasReturn = false, bool smallEndian = true) {
  // if hasReturn, then it's blr, otherwise it's br
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(0b1101011000011111000000, 10);
  opcode.setField(int(hasReturn), 21);
  opcode.setField(rn, 5, 5);
  return opcode.getInstruction();
}

// Reference: https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/TBNZ--Test-bit-and-branch-if-nonzero-
// Test bit and branch if nonzero
// This instruction compares the value of a bit in a general-purpose register with zero, and conditionally branches to a label at a PC-relative offset
// if the comparison is not equal. It provides a hint that this is not a subroutine call or return. This instruction does not affect condition flags.
// Usage: TBNZ <R><t>, #<imm>, <label>, label is encoded as "imm14" times 4.
// <t> is the number [0-30] of the general-purpose register to be tested or the name ZR (31), encoded in the "Rt" field.
string encodeTbnz(RegType regtype, uint8_t rt, uint8_t imm, uint16_t imm14) {
  auto opcode = Arm64Opcode();
  opcode.setSf(regtype);
  opcode.setField(0b110111, 24);
  opcode.setRt(rt);
  opcode.setImm14(imm14);
  return opcode.getInstruction();
}

string encodeAdr(uint8_t rd, int32_t imm, bool smallEndian = true) {
  // todo: need to check correctness!!
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(1, 28);
  opcode.setField(imm & 0x3, 29, 2);
  opcode.setField((imm >> 2) & 0x7FFFF, 5, 19);
  opcode.setRd(rd);
  return opcode.getInstruction();
}
