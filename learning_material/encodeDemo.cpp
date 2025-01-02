#include "../include/Opcode.hpp"
int main() {
      uint32_t inst = encodeBranch(1);
      std::cout << "Instruction: B, #1\n";
      std::cout << "Machine code (binary): " << toBinaryString(inst) << "\n";
      std::cout << "Machine code (hex)   : " << toHexString(inst) << "\n";
    //   uint32_t inst = encodeAddSubImm(/*isSub=*/true, 31, 31,
    //                                   /*imm=*/48,
    //                                   /*shift12=*/false);
    //   std::cout << "Instruction: sub sp, sp, #48\n";
    //   std::cout << "Machine code (binary): " << toBinaryString(inst) << "\n";
    //   std::cout << "Machine code (hex)   : " << toHexString(inst) << "\n";
    // }
    // {
    //   uint32_t inst = encodeLoadStoreUnsignedImm(LdStType::LDR_32,
    //                                              /*rt=*/2,
    //                                              /*rn=*/31,
    //                                              /*imm12=*/1);
    //   std::cout << "Instruction: ldr w2, [sp, #4]\n";
    //   std::cout << "Machine code (binary): " << toBinaryString(inst) << "\n";
    //   std::cout << "Machine code (hex)   : " << toHexString(inst) << "\n";
    // }
}