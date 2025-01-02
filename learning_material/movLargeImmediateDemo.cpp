#include "utils.h"

// 枚举寄存器类型
enum RegType { W_REG, X_REG };

// 编码MOVZ指令
uint32_t encodeMovz(uint8_t rd, uint16_t imm16, RegType regType, uint8_t hw, bool smallEndian = true) {
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
    instruction = __builtin_bswap32(instruction); // 转换为小端
  }
  return instruction;
}

// 编码MOVK指令
uint32_t encodeMovk(uint8_t rd, uint16_t imm16, RegType regType, uint8_t hw, bool smallEndian = true) {
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
// 分解64位整数为四个16位立即数
void decomposeInt64(int64_t value, uint16_t &imm0, uint16_t &imm1, uint16_t &imm2, uint16_t &imm3) {
  imm0 = value & 0xFFFF;         // 提取最低16位
  imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
  imm2 = (value >> 32) & 0xFFFF; // 提取第3个16位
  imm3 = (value >> 48) & 0xFFFF; // 提取最高16位
}

void decomposeInt32(int32_t value, uint16_t &imm0, uint16_t &imm1) {
  imm0 = value & 0xFFFF;         // 提取最低16位
  imm1 = (value >> 16) & 0xFFFF; // 提取第2个16位
}

int main() {
  int64_t number = 0x1234567890ABCDEF; // 示例整数
  uint16_t imm0, imm1, imm2, imm3;
  decomposeInt64(number, imm0, imm1, imm2, imm3);

  uint8_t rd = 0; // 目标寄存器 X0
  RegType regType = X_REG;
  cout << toHexString(encodeMovz(rd, imm0, regType, 0)).substr(2) << endl;
  cout << toHexString(encodeMovk(rd, imm1, regType, 1)).substr(2) << endl;
  cout << toHexString(encodeMovk(rd, imm2, regType, 2)).substr(2) << endl;
  cout << toHexString(encodeMovk(rd, imm3, regType, 3)).substr(2) << endl;

  return 0;
}
