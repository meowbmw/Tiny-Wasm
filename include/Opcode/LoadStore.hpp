#pragma once
#include "Base.hpp"

string encodeLdpStp(RegType regType, LdStType ldstType, uint8_t rt, uint8_t rt2, uint8_t rn, int32_t imm,
                    EncodingMode mode = EncodingMode::SignedOffset, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setSf(regType);
  int8_t imm7 = imm >> ((regType == X_REG) ? 3 : 2);
  opcode.setField((int)(ldstType == LDR), 22);
  switch (mode) {
  case EncodingMode::PreIndex:
    opcode.setField(0b1010011, 23);
    break;
  case EncodingMode::PostIndex:
    opcode.setField(0b1010001, 23);
    break;
  case EncodingMode::SignedOffset:
    opcode.setField(0b1010010, 23);
    break;
  default:
    throw "Unsupported Encode mode for LDP/STP";
  }
  opcode.setImm7(imm7);
  opcode.setRt2(rt2);
  opcode.setRn(rn);
  opcode.setRt(rt);
  return opcode.getInstruction();
}

string encodeLoadStoreImm(RegType regType, LdStType ldstType, uint8_t rt, uint8_t rn, int16_t imm, EncodingMode mode = EncodingMode::UnSignedOffset,
                          bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(1, 31);
  opcode.setField((int)(regType == X_REG), 30);
  opcode.setField(0b111, 27);
  opcode.setField((int)(ldstType == LDR), 22);
  opcode.setField((int)(mode == EncodingMode::UnSignedOffset), 24);
  switch (mode) {
  case EncodingMode::PostIndex:
    opcode.setImm9(imm);
    opcode.setField(1, 10);
    break;
  case EncodingMode::PreIndex:
    opcode.setImm9(imm);
    opcode.setField(0b11, 10);
    break;
  case EncodingMode::UnSignedOffset:
    opcode.setImm12(static_cast<uint16_t>(imm >> ((regType == X_REG) ? 3 : 2)));
    break;
  default:
    throw "Unsupported Encode mode for LDR/STR immediate";
  }
  opcode.setRt(rt);
  opcode.setRn(rn);
  return opcode.getInstruction();
}
