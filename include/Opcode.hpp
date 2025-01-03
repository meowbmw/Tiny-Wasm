#pragma once
#include "utils.h"

/**
 * TODO: work in progress
 * only implement very basic instruction format
 * currently support add/sub immediate; ldr/str unsigned offset
 *
 */
// 用来区分load/store及其宽度
enum class LdStType {
  STR_32,  // store w
  LDR_32,  // load w
  STR_64,  // store x
  LDR_64,  // load x
  STR_F32, // store s
  LDR_F32, // load s
  STR_F64, // store d
  LDR_F64  // load d
};

enum RegType { W_REG, X_REG };
uint32_t encodeBranch(uint32_t imm26, bool smallEndian = true) {
  uint32_t instruction = 0x14000000;
  instruction |= (imm26 & 0x3FFFFFF);
  if (smallEndian) {
    instruction = __builtin_bswap32(instruction); // convert to small endian
  }
  return instruction;
}

LdStType convertLdSt(LdStType type) {
  /**
   * Convert store type to read type
   * And vice versa.
   */
  switch (type) {
  case LdStType::STR_32:
    return LdStType::LDR_32;
  case LdStType::LDR_32:
    return LdStType::STR_32;
  case LdStType::STR_64:
    return LdStType::LDR_64;
  case LdStType::LDR_64:
    return LdStType::STR_64;
  case LdStType::STR_F32:
    return LdStType::LDR_F32;
  case LdStType::STR_F64:
    return LdStType::LDR_F64;
  case LdStType::LDR_F32:
    return LdStType::STR_F32;
  case LdStType::LDR_F64:
    return LdStType::STR_F64;
  default:
    throw "Unknown LdStType";
    return LdStType::LDR_32;
  }
}
// 将立即数转换为MOVZ指令的机器码
uint32_t encodeMovz(uint8_t rd, uint16_t imm16, RegType regType, uint8_t hw, bool smallEndian = true) {
  // mov immediate and set all other bytes to zero.
  if (hw > 3) {
    throw std::invalid_argument("hw must be between 0 and 3");
  }
  uint32_t instruction = 0;
  // 基本的MOVZ指令编码
  if (regType == X_REG) {
    instruction = 0xD2800000; // MOVZ for X registers
  } else {
    instruction = 0x52800000; // MOVZ for W registers
  }
  // 设置hw字段
  instruction |= (hw & 0x3) << 21;
  // 设置立即数
  instruction |= (imm16 & 0xFFFF) << 5;
  // 设置目标寄存器
  instruction |= (rd & 0x1F);
  if (smallEndian) {
    instruction = __builtin_bswap32(instruction); // convert to small endian
  }
  return instruction;
}

uint32_t encodeMovk(uint8_t rd, uint16_t imm16, RegType regType, uint8_t hw, bool smallEndian = true) {
  // mov immediate and set keep other bytes as they were.
  if (hw > 3) {
    throw std::invalid_argument("hw must be between 0 and 3");
  }
  uint32_t instruction = 0;
  // 基本的MOVK指令编码
  if (regType == X_REG) {
    instruction = 0xF2800000; // MOVK for X registers
  } else {
    instruction = 0x72800000; // MOVK for W registers
  }
  // 设置hw字段
  instruction |= (hw & 0x3) << 21;
  // 设置立即数
  instruction |= (imm16 & 0xFFFF) << 5;
  // 设置目标寄存器
  instruction |= (rd & 0x1F);
  if (smallEndian) {
    instruction = __builtin_bswap32(instruction); // 转换为小端
  }
  return instruction;
}

string WrapperEncodeMovInt32(uint8_t rd, uint32_t value, RegType regType, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  auto instr_0 = toHexString(encodeMovz(rd, imm0, regType, 0, smallEndian)).substr(2);
  auto instr_1 = toHexString(encodeMovk(rd, imm1, regType, 1, smallEndian)).substr(2);
  return instr_0 + instr_1;
}

string WrapperEncodeMovInt64(uint8_t rd, uint64_t value, RegType regType, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  uint16_t imm2 = (value >> 32) & 0xFFFF; // 提取第3个16位
  uint16_t imm3 = (value >> 48) & 0xFFFF; // 提取最高16位
  auto instr_0 = toHexString(encodeMovz(rd, imm0, regType, 0, smallEndian)).substr(2);
  auto instr_1 = toHexString(encodeMovk(rd, imm1, regType, 1, smallEndian)).substr(2);
  auto instr_2 = toHexString(encodeMovk(rd, imm2, regType, 2, smallEndian)).substr(2);
  auto instr_3 = toHexString(encodeMovk(rd, imm3, regType, 3, smallEndian)).substr(2);
  return instr_0 + instr_1 + instr_2 + instr_3;
}

uint32_t encodeAddSubImm(bool isSub, uint8_t rd, uint8_t rn, uint16_t imm, bool shift12, bool smallEndian = true) {
  // 1) 先确定高8位
  //    对于 64-bit ADD (S=0) => 0x91
  //    对于 64-bit SUB (S=0) => 0xD1
  uint32_t inst = (isSub ? 0xD1 : 0x91) << 24;

  // 2) shift 占 bits[23..22]
  uint32_t shiftVal = shift12 ? 1 : 0;
  inst |= (shiftVal << 22);

  // 3) imm12 占 bits[21..10]
  if (imm > 4095) {
    throw std::out_of_range("Immediate too large (max 4095).");
  }
  inst |= ((imm & 0xFFF) << 10);

  // 4) Rn 占 bits[9..5]
  if (rn > 31) {
    throw std::out_of_range("Rn register out of range.");
  }
  inst |= ((rn & 0x1F) << 5);

  // 5) Rd 占 bits[4..0]
  if (rd > 31) {
    throw std::out_of_range("Rd register out of range.");
  }
  inst |= (rd & 0x1F);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  return inst;
}
uint32_t encodeLoadStoreUnsignedImm(LdStType type, uint8_t rt, uint8_t rn, uint16_t imm12, bool smallEndian = true) {
  /*
    - V = 0 表示普通（非FP/NEON） load/store
    - opc
        00 -> store 32-bit (STR Wt)
        01 -> load 32-bit  (LDR Wt)
        10 -> store 64-bit (STR Xt)
        11 -> load 64-bit  (LDR Xt)
    - imm12 是 12位无符号立即数，一般乘以访问大小得到字节偏移
    例如 size=2 表示访问 4字节， size=3 表示访问 8字节， 所以实际 offset = imm12 << size

    - Rn：基址寄存器
    - Rt：目标寄存器

    //   uint32_t inst = encodeLoadStoreUnsignedImm(LdStType::LDR_32,
    //                                              rt=2,
    //                                              rn=31,
    //                                              imm12=1);
    //   std::cout << "Instruction: ldr w2, [sp, #4]\n";
  */
  // imm12 up to 4095
  if (imm12 > 4095) {
    throw std::out_of_range("Offset imm12 too large");
  }
  uint32_t base = 0;
  switch (type) {
  case LdStType::STR_32:
    base = 0xB9000000; // str w
    imm12 >>= 2;
    break;
  case LdStType::LDR_32:
    base = 0xB9400000; // ldr w
    imm12 >>= 2;
    break;
  case LdStType::STR_64:
    base = 0xF9000000; // str x
    imm12 >>= 3;
    break;
  case LdStType::LDR_64:
    base = 0xF9400000; // ldr x
    imm12 >>= 3;
    break;
  case LdStType::STR_F32:
    base = 0xBD000000; // ldr s
    imm12 >>= 2;
    break;
  case LdStType::STR_F64:
    base = 0xFD000000; // ldr d
    imm12 >>= 3;
    break;
  case LdStType::LDR_F32:
    base = 0xBD400000; // ldr s
    imm12 >>= 2;
    break;
  case LdStType::LDR_F64:
    base = 0xFD400000; // ldr d
    imm12 >>= 3;
    break;
  }

  // imm12 -> bits [21..10]
  // rn    -> bits [9..5]
  // rt    -> bits [4..0]
  uint32_t inst = base | ((imm12 & 0xFFF) << 10) | ((rn & 0x1F) << 5) | (rt & 0x1F);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // conver to small endian
  }
  return inst;
}
