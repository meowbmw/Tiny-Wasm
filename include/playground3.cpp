#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "utils.h"

enum FRegType { S_REG, D_REG };

// 将立即数转换为FMOV指令的机器码
uint32_t encode_fmov(uint8_t rd, float imm, FRegType regType, bool smallEndian = true) {
  uint32_t instruction = 0;
  uint32_t imm_bits = float_to_bits(imm);
//   cout << toBinaryString(imm_bits) << endl;

  if (regType == S_REG) {
    // FMOV for S registers (32-bit float)
    instruction = 0x1E201000; // FMOV S instruction base encoding
    // instruction |= (0x1E << 24);
    // instruction |= (2 << 20);
    cout << toBinaryString(instruction) << endl;
  } else {
    throw std::invalid_argument("Invalid register type for 32-bit float");
  }

  // 设置立即数
  instruction |= (imm_bits & 0xFF) << 13; // imm8
  instruction |= (imm_bits & 0x1F) << 5;  // imm5

  // 设置目标寄存器
  instruction |= (rd & 0x1F);
  if (smallEndian) {
    instruction = __builtin_bswap32(instruction); // convert to small endian
  }

  return instruction;
}

// 将立即数转换为FMOV指令的机器码
uint32_t encode_fmov(uint8_t rd, double imm, FRegType regType, bool smallEndian = true) {
  uint32_t instruction = 0;
  uint64_t imm_bits = double_to_bits(imm);

  if (regType == D_REG) {
    // FMOV for D registers (64-bit double)
    instruction = 0x1E601000; // FMOV D instruction base encoding
  } else {
    throw std::invalid_argument("Invalid register type for 64-bit double");
  }

  // 设置立即数
  instruction |= (imm_bits & 0xFF) << 13; // imm8
  instruction |= (imm_bits & 0x1F) << 5;  // imm5

  // 设置目标寄存器
  instruction |= (rd & 0x1F);
  if (smallEndian) {
    instruction = __builtin_bswap32(instruction); // convert to small endian
  }
  return instruction;
}

int main() {
  try {
    uint8_t rd = 31; // 目标寄存器S0或D0

    // 测试32位浮点数
    float imm_float = 4.0f;
    uint32_t fmov_s = encode_fmov(rd, imm_float, S_REG,false);
    cout << "Original assembly: fmov	s31, #4.000000000000000000e+00" << endl;
    std::cout << "Machine code (binary): " << toBinaryString(fmov_s) << "\n";
    cout << "Actual answer:         " << formatBinaryString(HexToBinary("1e22101f")) << endl;
    cout << "Original assembly: fmov	d31, #4.000000000000000000e+00" << endl;
    std::cout << "FMOV (S) Machine code: 0x" << std::hex << fmov_s << std::endl;
    cout << "Actual answer:         " << "0x1f10221e" << endl;

    // 测试64位浮点数
    // double imm_double = 4;
    // uint32_t fmov_d = encode_fmov(rd, imm_double, D_REG);
    // std::cout << "Machine code (binary): " << toBinaryString(fmov_d) << "\n";
    // std::cout << "FMOV (D) Machine code: 0x" << std::hex << fmov_d << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  return 0;
}