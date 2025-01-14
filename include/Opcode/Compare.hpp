#pragma once
#include "Arithmetic.hpp"
#include "Base.hpp"

string encodeCompareNegativeShift(RegType regType, uint8_t rn, uint8_t rm, uint8_t imm6 = 0, uint8_t shift = 0, bool smallEndian = true) {
  // CMN shifted register:
  // An alias of adds, only discards the result
  return encodeAddSubShift(false, regType, 31, rn, rm, imm6, shift, smallEndian);
}
string encodeCompareShift(RegType regType, uint8_t rn, uint8_t rm, uint8_t imm6 = 0, uint8_t shift = 0, bool smallEndian = true) {
  // CMP shifted register:
  // An alias of subs, only discards the result
  return encodeAddSubShift(true, regType, 31, rn, rm, imm6, shift, smallEndian);
}
string encodeCompareImm(RegType regType, uint8_t rn, uint16_t imm12, bool shift = 0, bool smallEndian = true) {
  return encodeAddSubImm(regType, true, 31, rn, imm12, shift, true, true, smallEndian);
}
string encodeCSEL(RegType regType, uint8_t rd, uint8_t rn, uint8_t rm, uint8_t cond, bool smallEndian = true) {
  /**
   * This instruction writes the value of the first source register to the destination register if the condition is TRUE. If the condition is FALSE,
   * it writes the value of the second source register to the destination register.
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/CSEL--Conditional-select-?lang=en
   */
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b110101, 23);
  opcode.setRd(rd);
  opcode.setRm(rm);
  opcode.setRn(rn);
  opcode.setCond(cond);
  return opcode.getInstruction();
}
