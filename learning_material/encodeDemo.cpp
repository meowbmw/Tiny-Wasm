#include <capstone/capstone.h>
#include "../include/Opcode.hpp"

int main() {
  uint32_t machine_code = 0xEA7D059B; // Example machine code for "mov x0, #1"
  encodeAdr(0, 12);
}
