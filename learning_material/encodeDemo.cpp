#include <capstone/capstone.h>

#include "../include/Opcode.hpp"

int main() {
  // encodeLoadStoreImm(W_REG, LDR, 10, 15, 16);
  // encodeByteLoadStoreImm(LDR, 10, 15, 5);
  encodeLSLImm(X_REG, 10, 10, 5);
}
