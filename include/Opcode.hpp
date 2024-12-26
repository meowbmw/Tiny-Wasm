#include "utils.h"

/**
 * TODO: work in progress
 * only implement very basic instruction format
 * currently support add/sub immediate; ldr/str unsigned offset
 *
 */
// 用来区分load/store及其宽度
enum class LdStType {
  STR_32, // store w
  LDR_32, // load w
  STR_64, // store x
  LDR_64,  // load x
  STR_F32, // store s
  LDR_F32, // load s
  STR_F64, // store d
  LDR_F64  // load d
};

enum RegType { W_REG, X_REG };

// 将立即数转换为MOVZ指令的机器码
uint32_t encode_movz(uint8_t rd, uint16_t imm16, RegType regType, uint8_t hw) {
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
  return instruction;
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
  */
  // imm12 up to 4095
  if (imm12 > 4095) {
    throw std::out_of_range("Offset imm12 too large");
  }
  uint32_t base = 0;
  switch (type) {
  case LdStType::STR_32:
    base = 0xB9000000; // str w
    break;
  case LdStType::LDR_32:
    base = 0xB9400000; // ldr w
    break;
  case LdStType::STR_64:
    base = 0xF9000000; // str x
    break;
  case LdStType::LDR_64:
    base = 0xF9400000; // ldr x
    break;
  case LdStType::STR_F32:
    base = 0xBD000000; // ldr s
    break;
  case LdStType::STR_F64:
    base = 0xFD000000; // ldr d
    break;
  case LdStType::LDR_F32:
    base = 0xBD400000; // ldr s
    break;
  case LdStType::LDR_F64:
    base = 0xFD400000; // ldr d
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
