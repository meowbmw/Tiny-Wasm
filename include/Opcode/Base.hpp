#pragma once
#include "../Utils.h"
/**
 * TODO: work in progress
 * only implement very basic instruction format
 * currently support add/sub immediate; ldr/str unsigned offset
 *
 */

enum class EncodingMode { PostIndex, PreIndex, SignedOffset, UnSignedOffset };
enum LdStType { STR, LDR };
enum RegType { W_REG, X_REG, S_REG, D_REG };
unordered_map<RegType, string> reg_char_map = {{W_REG, "w"}, {X_REG, "x"}, {S_REG, "s"}, {D_REG, "d"}};
const unordered_map<uint8_t, string> cond_str_map = {
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
const unordered_map<string, uint8_t> reverse_cond_str_map = {
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
auto getWasmType(const wasm_type &var) {
  if (std::holds_alternative<int32_t>(var)) {
    return W_REG;
  } else if (std::holds_alternative<int64_t>(var)) {
    return X_REG;
  } else if (std::holds_alternative<float>(var)) {
    return S_REG;
  } else if (std::holds_alternative<double>(var)) {
    return D_REG;
  } else {
    throw "Unknown type";
  }
}
class Arm64Opcode {
public:
  void makeAssmeblyString() {
    assemblyString = disassemble(inst);
  }
  void setField(auto val, int offset, int set_length = 32) {
    auto limit = (1LL << set_length) - 1;
    if (val > limit) {
      throw std::out_of_range("Encode value out of range.");
    }
    inst |= ((val & limit) << offset);
  }
  // set registers
  void setRm(auto rm) {
    setField(rm, 16, 5);
  }
  void setRn(auto rn) {
    setField(rn, 5, 5);
  }
  void setRd(auto rd) {
    setField(rd, 0, 5);
  }
  void setRt(auto rt) {
    setField(rt, 0, 5);
  }
  void setRt2(auto rt2) {
    setField(rt2, 10, 5);
  }
  // set immediate
  void setImm6(auto imm6) {
    setField(imm6, 10, 6);
  }
  void setImm7(auto imm7) {
    setField(imm7, 15, 7);
  }
  void setImm9(auto imm9) {
    setField(imm9, 12, 9);
  }
  void setImm12(auto imm12) {
    setField(imm12, 10, 12);
  }
  void setImm16(auto imm16) {
    setField(imm16, 5, 16);
  }
  void setImm19(auto imm19) {
    setField(imm19, 5, 19);
  }
  // set other fields
  void setSf(auto regType) {
    setField((int)(regType == X_REG), 31, 1);
  }
  void setCond(auto cond) {
    setField(cond, 12, 4);
  }
  void setShift(auto shift) {
    setField(shift, 22, 2);
  }
  void setSh(auto sh) {
    setField(sh, 22, 1);
  }
  void setHw(auto hw) {
    setField(hw, 21, 2);
  }
  void commonEncode() {
    if (smallEndian) {
      inst = __builtin_bswap32(inst); // convert to small endian
    }
    instruction = toHexString(inst).substr(2);
    if (enablePrint) {
      cout << format("       {} | {}", assemblyString, instruction) << endl;
    }
  }
  string getInstruction() {
    makeAssmeblyString();
    commonEncode();
    return instruction;
  }
  Arm64Opcode(bool endian = true, bool printFlag = true) {
    smallEndian = endian;
    enablePrint = printFlag;
  }
  uint32_t inst = 0;
  string assemblyString;
  string instruction;
  bool enablePrint = true;
  bool smallEndian = true;
};

string encodeNop(bool smallEndian = true) {
  auto opcode = Arm64Opcode(smallEndian);
  opcode.setField(0b11010101000000110010000000011111, 0);
  return opcode.getInstruction();
}