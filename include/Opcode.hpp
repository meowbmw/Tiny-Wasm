#pragma once
#include "Utils.h"

/**
 * TODO: work in progress
 * only implement very basic instruction format
 * currently support add/sub immediate; ldr/str unsigned offset
 *
 */

enum LdStType { STR, LDR };
enum RegType { W_REG, X_REG, S_REG, D_REG };
string common_encode(uint32_t inst) {
  return toHexString(inst).substr(2);
}
string encodeBranch(uint32_t imm26, bool smallEndian = true) {
  uint32_t inst = 0x14000000;
  inst |= (imm26 & 0x3FFFFFF);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: b {} | {}", imm26, instruction) << endl;
  return instruction;
}
// 将立即数转换为MOVZ指令的机器码
string encodeMovz(uint8_t rd, uint16_t imm16, RegType regType, uint8_t hw, bool smallEndian = true) {
  // mov immediate and set all other bytes to zero.
  if (hw > 3) {
    throw std::invalid_argument("hw must be between 0 and 3");
  }
  uint32_t inst = 0;
  // 基本的MOVZ指令编码
  if (regType == X_REG) {
    inst = 0xD2800000; // MOVZ for X registers
  } else {
    inst = 0x52800000; // MOVZ for W registers
  }
  // 设置hw字段
  inst |= (hw & 0x3) << 21;
  // 设置立即数
  inst |= (imm16 & 0xFFFF) << 5;
  // 设置目标寄存器
  inst |= (rd & 0x1F);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string reg_char = (regType == X_REG) ? "x" : "w";
  string instruction = common_encode(inst);
  cout << format("Emit: movz {}{}, #{}, lsl #{} | {}", reg_char, rd, imm16, hw * 16, instruction) << endl;
  return instruction;
}

string encodeMovk(uint8_t rd, uint16_t imm16, RegType regType, uint8_t hw, bool smallEndian = true) {
  // mov immediate and set keep other bytes as they were.
  if (hw > 3) {
    throw std::invalid_argument("hw must be between 0 and 3");
  }
  uint32_t inst = 0;
  // 基本的MOVK指令编码
  if (regType == X_REG) {
    inst = 0xF2800000; // MOVK for X registers
  } else {
    inst = 0x72800000; // MOVK for W registers
  }
  // 设置hw字段
  inst |= (hw & 0x3) << 21;
  // 设置立即数
  inst |= (imm16 & 0xFFFF) << 5;
  // 设置目标寄存器
  inst |= (rd & 0x1F);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // 转换为小端
  }
  string reg_char = (regType == X_REG) ? "x" : "w";
  string instruction = common_encode(inst);
  cout << format("Emit: movk {}{}, #{}, lsl #{} | {}", reg_char, rd, imm16, hw * 16, instruction) << endl;
  return instruction;
}

string WrapperEncodeMovInt32(uint8_t rd, uint32_t value, RegType regType, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  auto instr_0 = encodeMovz(rd, imm0, regType, 0, smallEndian);
  auto instr_1 = encodeMovk(rd, imm1, regType, 1, smallEndian);
  return instr_0 + instr_1;
}

string WrapperEncodeMovInt64(uint8_t rd, uint64_t value, RegType regType, bool smallEndian = true) {
  uint16_t imm0 = value & 0xFFFF;         // 提取最低16位
  uint16_t imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  uint16_t imm2 = (value >> 32) & 0xFFFF; // 提取第3个16位
  uint16_t imm3 = (value >> 48) & 0xFFFF; // 提取最高16位
  auto instr_0 = encodeMovz(rd, imm0, regType, 0, smallEndian);
  auto instr_1 = encodeMovk(rd, imm1, regType, 1, smallEndian);
  auto instr_2 = encodeMovk(rd, imm2, regType, 2, smallEndian);
  auto instr_3 = encodeMovk(rd, imm3, regType, 3, smallEndian);
  return instr_0 + instr_1 + instr_2 + instr_3;
}
string encodeAddSubShift(bool isSub, RegType regType, uint8_t rd, uint8_t rn, uint8_t rm, uint16_t imm6 = 0, uint8_t shift = 0,
                         bool smallEndian = true) {
  /**
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/ADD--shifted-register---Add-optionally-shifted-register-?lang=en#amount__5
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/SUB--shifted-register---Subtract-optionally-shifted-register-
   */
  string reg_char = (regType == X_REG) ? "x" : "w";
  // 1) 先确定高8位
  uint32_t inst = 0;
  if (regType == X_REG) {
    inst = 0x8B000000;
  } else {
    inst = 0x0B000000;
  }
  if (isSub) {
    inst |= (0x1 << 30);
  }
  // 2) shift 占 bits[23..22]
  if (shift > 3) {
    throw std::out_of_range("Shift value out of range (max 3).");
  }
  inst |= ((shift & 0x3) << 22);
  // 3) rm 占 bits[20..16]
  if (rm > 31) {
    throw std::out_of_range("Rm register out of range.");
  }
  inst |= ((rm & 0x1F) << 16);
  // 4) imm6 占 bits[15..10]
  if (imm6 > 63) {
    throw std::out_of_range("Immediate value out of range (max 63).");
  }
  inst |= ((imm6 & 0x3F) << 10);
  // 5) rn 占 bits[9..5]
  if (rn > 31) {
    throw std::out_of_range("Rn register out of range.");
  }
  inst |= ((rn & 0x1F) << 5);
  // 6) rd 占 bits[4..0]
  if (rd > 31) {
    throw std::out_of_range("Rd register out of range.");
  }
  inst |= (rd & 0x1F);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: {} {}{}, {}{}, {}{} | {}", ((isSub) ? "sub" : "add"), reg_char, rd, reg_char, rn, reg_char, rm, instruction) << endl;
  return instruction;
}
string encodeMul(RegType regType, uint8_t rd, uint8_t rn, uint8_t rm, bool smallEndian = true) {
  /*
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/MUL--Multiply--an-alias-of-MADD-
   */
  string reg_char = (regType == X_REG) ? "x" : "w";
  uint32_t inst = 0b00011011000000000111110000000000;
  if (regType == X_REG) {
    inst |= (1 << 31);
  }
  if (rm > 31) {
    throw std::out_of_range("Rm register out of range.");
  }
  inst |= ((rm & 0x1F) << 16);
  if (rn > 31) {
    throw std::out_of_range("Rn register out of range.");
  }
  inst |= ((rn & 0x1F) << 5);
  if (rd > 31) {
    throw std::out_of_range("Rd register out of range.");
  }
  inst |= (rd & 0x1F);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: mul {}{}, {}{}, {}{} | {}", reg_char, rd, reg_char, rn, reg_char, rm, instruction) << endl;
  return instruction;
}
string encodeAddSubImm(RegType regType, bool isSub, uint8_t rd, uint8_t rn, uint16_t imm, bool shift12 = false, bool smallEndian = true) {
  uint32_t inst = 0;
  if (regType == X_REG) {
    inst |= (1 << 31);
  }
  if (isSub) {
    inst |= (1 << 30);
  }
  inst |= (0b10001 << 24);
  // 2) shift 占 bits[22..22]
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
  string instruction = common_encode(inst);
  string reg_char = (regType == X_REG) ? "x" : "w";
  string rd_str = (rd == 31) ? "sp" : (reg_char + to_string(rd));
  string rn_str = (rn == 31) ? "sp" : (reg_char + to_string(rn));
  cout << format("Emit: {} {}, {}, #{} | {}", (isSub ? "sub" : "add"), rd_str, rn_str, imm, instruction) << endl;
  return instruction;
}
string encodeLoadStoreUnsignedImm(RegType regType, LdStType ldstType, uint8_t rt, uint8_t rn, uint16_t imm12, bool smallEndian = true) {
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
  string reg_char;
  string ldstStr;
  string immstr = to_string(imm12);
  if (ldstType != LdStType::LDR) {
    if (ldstType != LdStType::STR) {
      err("Unknown LdStType");
    }
  }
  if (regType == RegType::W_REG) {
    if (ldstType == LdStType::LDR) {
      base = 0xB9400000; // ldr w
    } else if (ldstType == LdStType::STR) {
      base = 0xB9000000; // str w
    }
    reg_char = "w";
    imm12 >>= 2;
  } else if (regType == RegType::X_REG) {
    if (ldstType == LdStType::LDR) {
      base = 0xF9400000; // ldr x
    } else if (ldstType == LdStType::STR) {
      base = 0xF9000000; // str x
    }
    reg_char = "x";
    imm12 >>= 3;
  } else if (regType == RegType::S_REG) {
    if (ldstType == LdStType::LDR) {
      base = 0xBD400000; // ldr s
    } else if (ldstType == LdStType::STR) {
      base = 0xBD000000; // str s
    }
    reg_char = "s";
    imm12 >>= 2;
  } else if (regType == RegType::D_REG) {
    if (ldstType == LdStType::LDR) {
      base = 0xFD400000; // ldr d
    } else if (ldstType == LdStType::STR) {
      base = 0xFD000000; // str d
    }
    reg_char = "d";
    imm12 >>= 3;
  } else {
    err("Unknown register type");
  }
  // imm12 -> bits [21..10]
  // rn    -> bits [9..5]
  // rt    -> bits [4..0]
  uint32_t inst = base | ((imm12 & 0xFFF) << 10) | ((rn & 0x1F) << 5) | (rt & 0x1F);
  if (ldstType == STR) {
    ldstStr = "str";
  } else {
    ldstStr = "ldr";
  }
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // conver to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: {} {}{}, [{}, #{}] | {}", ldstStr, reg_char, rt, ((rn == 31) ? "sp" : to_string(rn)), immstr, instruction) << endl;
  return instruction;
}
