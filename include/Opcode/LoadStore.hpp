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

string commonLoadStoreImm(RegType regType, LdStType ldstType, int operateSize, uint8_t rt, uint8_t rn, int16_t imm,
                          EncodingMode mode = EncodingMode::UnSignedOffset, bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(operateSize / 16, 30);
  opcode.setField(0b111, 27);
  opcode.setField((int)(ldstType == LDR), 22);
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
    opcode.setField(1, 24);
    opcode.setImm12(static_cast<uint16_t>(imm >> (operateSize / 16)));
    break;
  default:
    throw "Unsupported Encode mode for LDR/STR immediate";
  }
  opcode.setRt(rt);
  opcode.setRn(rn);
  return opcode.getInstruction();
}
/**
 * Usage: LDR <Wt>, [<Xn|SP>], #<simm>
 * encodeLoadStoreImm(X_REG, LDR, 0, 31, 0x10); // LDR X0, [SP, #0x10]
 * encodeLoadStoreImm(W_REG, STR, 0, 31, 0x10); // STR W0, [SP, #0x10]
 * encodeLoadStoreImm(X_REG, LDR, 0, 31, 0x10, EncodingMode::PreIndex); // LDR X0, [SP, #0x10]!
 * encodeLoadStoreImm(X_REG, LDR, 0, 31, 0x10, EncodingMode::PostIndex); // LDR X0, [SP], #0x10
 *
 */
string encodeLoadStoreImm(RegType regType, LdStType ldstType, uint8_t rt, uint8_t rn, int16_t imm, EncodingMode mode = EncodingMode::UnSignedOffset,
                          bool smallEndian = true) {
  return commonLoadStoreImm(regType, ldstType, (regType == X_REG) ? 48 : 32, rt, rn, imm, mode, smallEndian);
}

string encodeWordLoadStoreImm(LdStType ldstType, uint8_t rt, uint8_t rn, int16_t imm, EncodingMode mode = EncodingMode::UnSignedOffset,
                              bool smallEndian = true) {
  return commonLoadStoreImm(W_REG, ldstType, 16, rt, rn, imm, mode, smallEndian);
}

string encodeByteLoadStoreImm(LdStType ldstType, uint8_t rt, uint8_t rn, int16_t imm, EncodingMode mode = EncodingMode::UnSignedOffset,
                              bool smallEndian = true) {
  return commonLoadStoreImm(W_REG, ldstType, 0, rt, rn, imm, mode, smallEndian);
}

string commonLoadStoreReg(RegType regType, LdStType ldstType, int operateSize, uint8_t rt, uint8_t rn, uint8_t rm, int option = 0b011,
                          bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(operateSize / 16, 30);
  opcode.setField(0b111, 27);
  opcode.setField((int)(ldstType == LDR), 22);
  opcode.setField(1, 21);
  /**
   * Is the index extend/shift specifier, defaulting to LSL, and which must be omitted for the LSL option when <amount> is omitted, encoded in option:
    option	<extend>
    010	UXTW
    011	LSL
    110	SXTW
    111	SXTX
   */
  opcode.setOption(option);
  opcode.setField(1, 11);
  opcode.setRm(rm);
  opcode.setRn(rn);
  opcode.setRt(rt);
  return opcode.getInstruction();
}
/**
 * This is different from load store imm, i.e.
 * LDR Rt, [Rn, Rm]
 */
string encodeLoadStoreReg(RegType regType, LdStType ldstType, uint8_t rt, uint8_t rn, uint8_t rm, int option = 0b011, bool smallEndian = true) {
  /**
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/LDR--register---Load-register--register--?lang=en
   */
  return commonLoadStoreReg(regType, ldstType, (regType == X_REG) ? 48 : 32, rt, rn, rm, option, smallEndian);
}

string encodeWordLoadStoreReg(LdStType ldstType, uint8_t rt, uint8_t rn, uint8_t rm, int option = 0b011, bool smallEndian = true) {
  return commonLoadStoreReg(W_REG, ldstType, 16, rt, rn, rm, option, smallEndian);
}

string encodeByteLoadStoreReg(LdStType ldstType, uint8_t rt, uint8_t rn, uint8_t rm, int option = 0b011, bool smallEndian = true) {
  return commonLoadStoreReg(W_REG, ldstType, 0, rt, rn, rm, option, smallEndian);
}
