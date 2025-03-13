#pragma once
#include "Arithmetic.hpp"
#include "Base.hpp"

string encodeMovSP(RegType regType, uint8_t rd, uint8_t rn, bool smallEndian = true) {
  return encodeAddSubImm(regType, false, rd, rn, 0);
}
string encodeMovRegister(RegType regType, uint8_t rd, uint8_t rm, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setRm(rm);
  opcode.setRd(rd);
  opcode.setField(0b10101, 25);
  opcode.setRn(0b11111);
  return opcode.getInstruction();
}

string encodeMovz(RegType regType, uint8_t rd, uint16_t imm16, uint8_t shift = 0, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b10100101, 23);
  opcode.setHw(shift >> 4);
  opcode.setImm16(imm16);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

string encodeMovk(RegType regType, uint8_t rd, uint16_t imm16, uint8_t shift, bool smallEndian = true) {
  // mov immediate and set keep other bytes as they were.
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b11100101, 23);
  opcode.setHw(shift >> 4);
  opcode.setImm16(imm16);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

string encodeMovn(uint8_t rd, uint16_t imm16, RegType regType, uint8_t shift, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b100101, 23);
  opcode.setHw(shift >> 4);
  opcode.setImm16(imm16);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

string WrapperEncodeMovInt32(uint8_t rd, int32_t value, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  auto instr_0 = encodeMovz(W_REG, rd, imm0, 0, smallEndian);
  auto instr_1 = encodeMovk(W_REG, rd, imm1, 1 * 16, smallEndian);
  return instr_0 + instr_1;
}

string WrapperEncodeMovInt64(uint8_t rd, int64_t value, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  uint16_t imm2 = (value >> 32) & 0xFFFF; // 提取第3个16位
  uint16_t imm3 = (value >> 48) & 0xFFFF; // 提取最高16位
  auto instr_0 = encodeMovz(X_REG, rd, imm0, 0, smallEndian);
  auto instr_1 = encodeMovk(X_REG, rd, imm1, 1 * 16, smallEndian);
  auto instr_2 = encodeMovk(X_REG, rd, imm2, 2 * 16, smallEndian);
  auto instr_3 = encodeMovk(X_REG, rd, imm3, 3 * 16, smallEndian);
  return instr_0 + instr_1 + instr_2 + instr_3;
}