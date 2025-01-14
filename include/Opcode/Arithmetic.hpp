#pragma once
#include "Base.hpp"

string encodeAddSubShift(bool isSub, RegType regType, uint8_t rd, uint8_t rn, uint8_t rm, uint8_t imm = 0, uint8_t shift = 0, bool isCmp = false,
                         bool smallEndian = true) {
  /**
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/ADD--shifted-register---Add-optionally-shifted-register-?lang=en#amount__5
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/SUB--shifted-register---Subtract-optionally-shifted-register-
   */
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b1011, 24);
  opcode.setField((int)(isSub), 30);
  opcode.setShift(shift);
  opcode.setRm(rm);
  opcode.setRn(rn);
  opcode.setRd(rd);
  opcode.setImm6(imm);
  return opcode.getInstruction();
}

string encodeMul(RegType regType, uint8_t rd, uint8_t rn, uint8_t rm, bool smallEndian = true) {
  /*
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/MUL--Multiply--an-alias-of-MADD-
   */
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b00011011000000000111110000000000, 0);
  opcode.setRm(rm);
  opcode.setRn(rn);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

string encodeDiv(RegType regType, bool isSigned, uint8_t rd, uint8_t rn, uint8_t rm, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b1101011, 22);
  opcode.setField(1, 11);
  opcode.setField((int)(isSigned), 10);
  opcode.setRm(rm);
  opcode.setRn(rn);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

string encodeAddSubImm(RegType regType, bool isSub, uint8_t rd, uint8_t rn, uint16_t imm, uint8_t shift = 0, bool isCmp = false, bool setFlag = false,
                       bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField((int)(isSub), 30);
  opcode.setField((int)(setFlag), 29);
  opcode.setField(0b10001, 24);
  opcode.setSh(shift);
  opcode.setImm12(imm);
  opcode.setRn(rn);
  opcode.setRd(rd);
  return opcode.getInstruction();
}
