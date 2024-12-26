/**
 * hw 字段
hw 字段（halfword）是一个 2 位的字段，用于指定 16 位立即数在目标寄存器中的位置。hw 字段的值表示立即数的 16
位部分在目标寄存器中的位置，通过移位操作实现：

00：表示立即数的 16 位部分放在目标寄存器的最低 16 位（不移位）。
01：表示立即数的 16 位部分放在目标寄存器的 16 到 31 位（左移 16 位）。
10：表示立即数的 16 位部分放在目标寄存器的 32 到 47 位（左移 32 位）。
11：表示立即数的 16 位部分放在目标寄存器的 48 到 63 位（左移 48 位）。
示例说明
假设我们有一个 64 位的目标寄存器 X0，并且我们想要加载一个立即数 0x1234 到寄存器的不同位置。我们可以使用 MOVZ 指令来实现这一点：

MOVZ X0, #0x1234, LSL #0   // hw = 00, 将立即数 0x1234 放在 X0 的最低 16 位
MOVZ X0, #0x1234, LSL #16  // hw = 01, 将立即数 0x1234 放在 X0 的 16 到 31 位
MOVZ X0, #0x1234, LSL #32  // hw = 10, 将立即数 0x1234 放在 X0 的 32 到 47 位
MOVZ X0, #0x1234, LSL #48  // hw = 11, 将立即数 0x1234 放在 X0 的 48 到 63 位
 */

#include <cstdint>
#include <iostream>
#include <stdexcept>

// 定义寄存器类型
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

int main() {
  try {
    uint8_t rd = 0;          // 目标寄存器X0或W0
    uint16_t imm16 = 0x1234; // 立即数

    // MOVZ组合来处理64位立即数
    uint32_t movz0 = encode_movz(rd, imm16, X_REG, 0); // hw = 0
    uint32_t movz1 = encode_movz(rd, imm16, X_REG, 1); // hw = 1
    uint32_t movz2 = encode_movz(rd, imm16, X_REG, 2); // hw = 2
    uint32_t movz3 = encode_movz(rd, imm16, X_REG, 3); // hw = 3

    std::cout << "MOVZ Machine code (hw=0): 0x" << std::hex << movz0 << std::endl;
    std::cout << "MOVZ Machine code (hw=1): 0x" << std::hex << movz1 << std::endl;
    std::cout << "MOVZ Machine code (hw=2): 0x" << std::hex << movz2 << std::endl;
    std::cout << "MOVZ Machine code (hw=3): 0x" << std::hex << movz3 << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  return 0;
}