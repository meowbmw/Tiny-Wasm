#include <capstone/capstone.h>

#include "../include/Opcode/Arithmetic.hpp"
#include "../include/Opcode/Base.hpp"
#include "../include/Opcode/Branch.hpp"

int main() {
  cout << (16 >> 4) << endl;
  uint32_t machine_code = 0xEA7D059B; // Example machine code for "mov x0, #1"
  encodeAddSubShift(false, W_REG, 10, 15, 16);
  encodeMul(X_REG, 10, 15, 5);
  encodeDiv(X_REG, true, 10, 15, 5);
  encodeAddSubImm(X_REG, true, 10, 15, 5);
  encodeBranchRegister(4);
}
