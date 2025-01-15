#pragma once
#include "Base.hpp"

struct BranchType {
  BranchType(bool with_condition = false, bool with_link = false, uint8_t Cond = 0) {
    withCondition = with_condition;
    withLink = with_link;
    cond = Cond;
  }
  bool withCondition;
  bool withLink;
  uint8_t cond;
};

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
