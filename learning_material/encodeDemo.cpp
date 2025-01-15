#include <capstone/capstone.h>

#include "../include/Opcode/Arithmetic.hpp"
#include "../include/Opcode/Base.hpp"
#include "../include/Opcode/Branch.hpp"

int main() {
  string s1 = "FFFFFFFFF135000A9155801A9176002A9196803A91B7004A91D7805A9E2030091023400F900008052C0035FD6";
  s1.replace(0, 9, string("0B000014"));
  cout << s1 << endl;
  cout << (16 >> 4) << endl;
  uint32_t machine_code = 0xEA7D059B; // Example machine code for "mov x0, #1"
  encodeAddSubShift(false, W_REG, 10, 15, 16);
  encodeMul(X_REG, 10, 15, 5);
  encodeDiv(X_REG, true, 10, 15, 5);
  encodeAddSubImm(X_REG, true, 10, 15, 5);
  encodeBranchRegister(4);
}
