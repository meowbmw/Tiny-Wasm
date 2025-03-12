#pragma once
#include "Base.hpp"

// SVC
// This instruction causes an exception to be taken to EL1.
// On executing an SVC instruction, the PE records the exception as a Supervisor Call exception in ESR_ELx, using the EC value 0x15, and the value of
// the immediate argument.
string encodeSVC(uint16_t imm16) {
  auto opcode = Arm64Opcode();
  opcode.setField(0b110101, 26);
  opcode.setImm16(imm16);
  opcode.setField(1, 0);
  return opcode.getInstruction();
}