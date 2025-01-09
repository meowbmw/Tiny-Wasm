#pragma once
#include "Utils.h"

/**
 * TODO: work in progress
 * only implement very basic instruction format
 * currently support add/sub immediate; ldr/str unsigned offset
 *
 */

enum class EncodingMode { PostIndex, PreIndex, SignedOffset };
enum LdStType { STR, LDR };
enum RegType { W_REG, X_REG, S_REG, D_REG };
map<uint8_t, string> cond_str_map = {
    // for print and debug purpose
    {0b0000, "eq"}, // Equal
    {0b0001, "ne"}, // Not equal
    {0b0010, "cs"}, // Carry set/unsigned higher or same
    {0b0011, "cc"}, // Carry clear/unsigned lower
    {0b0100, "mi"}, // Minus/negative
    {0b0101, "pl"}, // Plus/positive or zero
    {0b0110, "vs"}, // Overflow
    {0b0111, "vc"}, // No overflow
    {0b1000, "hi"}, // Unsigned higher
    {0b1001, "ls"}, // Unsigned lower or same
    {0b1010, "ge"}, // Signed greater than or equal
    {0b1011, "lt"}, // Signed less than
    {0b1100, "gt"}, // Signed greater than
    {0b1101, "le"}, // Signed less than or equal
    {0b1110, "al"}, // Always (unconditional)
    {0b1111, "nv"}  // Never (reserved)
};
map<string, uint8_t> reverse_cond_str_map = {
    {"eq", 0b0000}, // Equal
    {"ne", 0b0001}, // Not equal
    {"cs", 0b0010}, // Carry set/unsigned higher or same
    {"cc", 0b0011}, // Carry clear/unsigned lower
    {"mi", 0b0100}, // Minus/negative
    {"pl", 0b0101}, // Plus/positive or zero
    {"vs", 0b0110}, // Overflow
    {"vc", 0b0111}, // No overflow
    {"hi", 0b1000}, // Unsigned higher
    {"ls", 0b1001}, // Unsigned lower or same
    {"ge", 0b1010}, // Signed greater than or equal
    {"lt", 0b1011}, // Signed less than
    {"gt", 0b1100}, // Signed greater than
    {"le", 0b1101}, // Signed less than or equal
    {"al", 0b1110}, // Always (unconditional)
    {"nv", 0b1111}  // Never (reserved)
};
string common_encode(uint32_t inst) {
  return toHexString(inst).substr(2);
}
string encodeBranch(uint32_t offset, bool withLink = false, bool smallEndian = true) {
  // offset is instruction level, i.e. forward 5 instruction => offset = 5
  // this is different from the origin b label!!
  uint32_t inst = 0;
  inst |= (0b101 << 26);
  if (withLink) {
    inst |= (1 << 31);
  }
  inst |= (offset & 0x3FFFFFF);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: {} with offset {} | {}", (withLink ? "bl" : "b"), offset, instruction) << endl;
  return instruction;
}
string encodeLdpStp(RegType regType, LdStType ldstType, uint8_t rt, uint8_t rt2, uint8_t rn, int16_t imm,
                    EncodingMode mode = EncodingMode::SignedOffset, bool smallEndian = true) {
  string reg_char = (regType == X_REG) ? "x" : "w";
  int16_t imm7 = imm;
  uint32_t inst = 0;
  string instr_name;
  if (regType == X_REG) {
    imm7 >>= 3;
    inst |= (1 << 31);
  } else {
    imm7 >>= 2;
  }
  if (ldstType == LDR) {
    inst |= (1 << 22);
    instr_name = "ldp";
  } else {
    instr_name = "stp";
  }
  if (mode == EncodingMode::PreIndex) {
    inst |= (0b1010011 << 23);
  } else if (mode == EncodingMode::PostIndex) {
    inst |= (0b1010001 << 23);
  } else if (mode == EncodingMode::SignedOffset) {
    inst |= (0b1010010 << 23);
  } else {
    throw "Unsupported Encode mode for LDP/STP";
  }
  if (imm7 > 127 || imm7 < -128) {
    throw std::out_of_range("Imm7 out of range.");
  }
  inst |= ((imm7 & 0x7F) << 15);
  if (rt2 > 31) {
    throw std::out_of_range("Rt2 register out of range.");
  }
  inst |= ((rt2 & 0x1F) << 10);
  if (rn > 31) {
    throw std::out_of_range("Rn register out of range.");
  }
  inst |= ((rn & 0x1F) << 5);
  if (rt > 31) {
    throw std::out_of_range("Rt register out of range.");
  }
  inst |= (rt & 0x1F);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  if (mode == EncodingMode::PreIndex) {
    cout << format("Emit: {} {}{}, {}{}, [{}, #{}]! | {}", instr_name, reg_char, rt, reg_char, rt2, ((rn == 31) ? "sp" : to_string(rn)), imm,
                   instruction)
         << endl;
  } else if (mode == EncodingMode::PostIndex) {
    cout << format("Emit: {} {}{}, {}{}, [{}], #{} | {}", instr_name, reg_char, rt, reg_char, rt2, ((rn == 31) ? "sp" : to_string(rn)), imm,
                   instruction)
         << endl;
  } else if (mode == EncodingMode::SignedOffset) {
    cout << format("Emit: {} {}{}, {}{}, [{}, #{}] | {}", instr_name, reg_char, rt, reg_char, rt2, ((rn == 31) ? "sp" : to_string(rn)), imm,
                   instruction)
         << endl;
  }
  return instruction;
}
string encodeBranchCondition(uint32_t offset, uint8_t cond, bool smallEndian = true) {
  uint32_t inst = 0;
  inst |= (0b01010100 << 24);
  inst |= (offset << 5);
  inst |= (cond & 0xF);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: b.{} with {} | {}", cond_str_map[cond], offset, instruction) << endl;
  return instruction;
}
string encodeCSEL(RegType regType, uint8_t rd, uint8_t rn, uint8_t rm, uint8_t cond, bool smallEndian = true) {
  /**
   * This instruction writes the value of the first source register to the destination register if the condition is TRUE. If the condition is FALSE,
   * it writes the value of the second source register to the destination register.
   * https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/CSEL--Conditional-select-?lang=en
   */
  string reg_char = (regType == X_REG) ? "x" : "w";
  uint32_t inst = 0;
  if (regType == X_REG) {
    inst |= (1 << 31);
  }
  inst |= (0b110101 << 23);
  if (rd > 31) {
    throw std::out_of_range("Rd register out of range.");
  }
  inst |= (rd & 0x1F);
  if (rm > 31) {
    throw std::out_of_range("Rm register out of range.");
  }
  inst |= ((rm & 0x1F) << 16);
  if (rn > 31) {
    throw std::out_of_range("Rn register out of range.");
  }
  inst |= ((rn & 0x1F) << 5);
  if (cond > 15) {
    throw std::out_of_range("Condition out of range.");
  }
  inst |= ((cond & 0xF) << 12);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: csel {}{}, {}{}, {}{}, {} | {}", reg_char, rd, reg_char, rn, reg_char, rm, cond_str_map[cond], instruction) << endl;
  return instruction;
}
string encodeBranchRegister(uint8_t rn, bool hasReturn = false, bool smallEndian = true) {
  uint32_t inst = 0;
  inst |= (0b1101011000011111000000 << 10);
  if (hasReturn) {
    inst |= (1 << 21);
  }
  if (rn > 31) {
    throw std::out_of_range("Rn register out of range.");
  }
  inst |= ((rn & 0x1F) << 5);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: b{}r x{} | {}", (hasReturn ? "l" : ""), ((rn == 31) ? "sp" : to_string(rn)), instruction) << endl;
  return instruction;
}
string encodeMovRegister(RegType regType, uint8_t rd, uint8_t rm, bool smallEndian = true) {
  string reg_char = (regType == X_REG) ? "x" : "w";
  uint32_t inst = 0;
  if (regType == X_REG) {
    inst |= (1 << 31);
  }
  if (rd > 31) {
    throw std::out_of_range("Rd register out of range.");
  }
  inst |= (rd & 0x1F);
  if (rm > 31) {
    throw std::out_of_range("Rm register out of range.");
  }
  inst |= ((rm & 0x1F) << 16);
  inst |= (0b10101 << 25);
  inst |= (0b11111 << 5);
  if (smallEndian) {
    inst = __builtin_bswap32(inst); // convert to small endian
  }
  string instruction = common_encode(inst);
  cout << format("Emit: mov {}, {} | {}", ((rd == 31) ? reg_char + "zr" : reg_char + to_string(rd)),
                 ((rm == 31) ? reg_char + "zr" : reg_char + to_string(rm)), instruction)
       << endl;
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
                         bool isCompare = false, bool smallEndian = true) {
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
  string instr_name = "unknown";
  if (isCompare) {
    if (isSub) {
      instr_name = "cmp";
    } else {
      instr_name = "cmn";
    }
  } else {
    if (isSub) {
      instr_name = "sub";
    } else {
      instr_name = "add";
    }
  }
  cout << format("Emit: {} {}{}, {}{}, {}{} | {}", instr_name, reg_char, rd, reg_char, rn, reg_char, rm, instruction) << endl;
  return instruction;
}
string encodeCompareNegativeShift(RegType regType, uint8_t rn, uint8_t rm, uint16_t imm6 = 0, uint8_t shift = 0, bool smallEndian = true) {
  // CMN shifted register:
  // An alias of adds, only discards the result
  return encodeAddSubShift(false, regType, 31, rn, rm, imm6, shift, smallEndian);
}
string encodeCompareShift(RegType regType, uint8_t rn, uint8_t rm, uint16_t imm6 = 0, uint8_t shift = 0, bool smallEndian = true) {
  // CMP shifted register:
  // An alias of subs, only discards the result
  return encodeAddSubShift(true, regType, 31, rn, rm, imm6, shift, smallEndian);
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
string encodeDiv(RegType regType, bool isSigned, uint8_t rd, uint8_t rn, uint8_t rm, bool smallEndian = true) {
  string reg_char = (regType == X_REG) ? "x" : "w";
  uint32_t inst = 0;
  if (regType == X_REG) {
    inst |= (1 << 31);
  }
  inst |= (0b1101011 << 22);
  inst |= (1 << 11);
  if (isSigned == true) {
    inst |= (1 << 10);
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
  cout << format("Emit: {}div {}{}, {}{}, {}{} | {}", ((isSigned == true) ? "s" : "u"), reg_char, rd, reg_char, rn, reg_char, rm, instruction)
       << endl;
  return instruction;
}
string encodeAddSubImm(RegType regType, bool isSub, uint8_t rd, uint8_t rn, uint16_t imm, bool shift12 = false, bool isCompare = false,
                       bool setFlag = false, bool smallEndian = true) {
  uint32_t inst = 0;
  if (regType == X_REG) {
    inst |= (1 << 31);
  }
  if (isSub) {
    inst |= (1 << 30);
  }
  if (setFlag) {
    inst |= (1 << 29);
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
  if (isCompare) {
    // TODO: add shift print
    cout << format("Emit: cmp {}{}, #{} | {}", reg_char, rn, imm, instruction) << endl;
  } else {
    string rd_str = (rd == 31) ? "sp" : (reg_char + to_string(rd));
    string rn_str = (rn == 31) ? "sp" : (reg_char + to_string(rn));
    cout << format("Emit: {}{} {}, {}, #{} | {}", (isSub ? "sub" : "add"), (setFlag ? "s" : ""), rd_str, rn_str, imm, instruction) << endl;
  }
  return instruction;
}
string encodeCompareImm(RegType regType, uint8_t rn, uint8_t imm12, bool shift = 0, bool smallEndian = true) {
  return encodeAddSubImm(regType, true, 31, rn, imm12, shift, true, true, smallEndian);
}
string encodeMovSP(RegType regType, uint8_t rd, uint8_t rn, bool smallEndian = true) {
  string reg_char = (regType == X_REG) ? "x" : "w";
  streambuf *old = cout.rdbuf();
  cout.rdbuf(0);
  string instruction = encodeAddSubImm(regType, false, rd, rn, 0);
  cout.rdbuf(old);
  cout << format("Emit: mov {}, {} | {}", ((rd == 31) ? "sp" : reg_char + to_string(rd)), ((rn == 31) ? "sp" : reg_char + to_string(rn)), instruction)
       << endl;
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
  cout << format("Emit: {} {}{}, [{}, #{}] | {}", ldstStr, reg_char, rt, ((rn == 31) ? "sp" : reg_char + to_string(rn)), immstr, instruction) << endl;
  return instruction;
}

string encodeReturn() {
  const string instr = "C0035FD6";
  cout << format("Emit: ret | {}", instr) << endl;
  return instr;
}
