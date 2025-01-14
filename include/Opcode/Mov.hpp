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
// 将立即数转换为MOVZ指令的机器码
string encodeMovz(uint8_t rd, uint16_t imm16, RegType regType, uint8_t shift, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b10100101, 23);
  opcode.setHw(shift >> 4);
  opcode.setImm16(imm16);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

string encodeMovk(uint8_t rd, uint16_t imm16, RegType regType, uint8_t shift, bool smallEndian = true) {
  // mov immediate and set keep other bytes as they were.
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  opcode.setField(0b11100101, 23);
  opcode.setHw(shift >> 4);
  opcode.setImm16(imm16);
  opcode.setRd(rd);
  return opcode.getInstruction();
}

string WrapperEncodeMovInt32(uint8_t rd, uint32_t value, RegType regType, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  auto instr_0 = encodeMovz(rd, imm0, regType, 0, smallEndian);
  auto instr_1 = encodeMovk(rd, imm1, regType, 1 * 16, smallEndian);
  return instr_0 + instr_1;
}

string WrapperEncodeMovInt64(uint8_t rd, uint64_t value, RegType regType, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  uint16_t imm2 = (value >> 32) & 0xFFFF; // 提取第3个16位
  uint16_t imm3 = (value >> 48) & 0xFFFF; // 提取最高16位
  auto instr_0 = encodeMovz(rd, imm0, regType, 0, smallEndian);
  auto instr_1 = encodeMovk(rd, imm1, regType, 1 * 16, smallEndian);
  auto instr_2 = encodeMovk(rd, imm2, regType, 2 * 16, smallEndian);
  auto instr_3 = encodeMovk(rd, imm3, regType, 3 * 16, smallEndian);
  return instr_0 + instr_1 + instr_2 + instr_3;
}