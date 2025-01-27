#include <capstone/capstone.h>

#include "../include/Opcode.hpp"

int main() {
  /*
  https://github.com/sunfishcode/wasm-reference-manual/blob/master/WebAssembly.md#type-encoding-type
  */
  cout << "i32: " << hex << decodeSLEB128({(uint8_t)(-0x01)}, 7) << endl;
  cout << "i64: " << hex << decodeSLEB128({(uint8_t)(-0x02)}, 7) << endl;
  cout << "f32: " << hex << decodeSLEB128({(uint8_t)(-0x03)}, 7) << endl;
  cout << "f64: " << hex << decodeSLEB128({(uint8_t)(-0x04)}, 7) << endl;
  cout << "funcref: " << hex << decodeSLEB128({(uint8_t)(-0x10)}, 7) << endl;
  cout << "func: " << hex << decodeSLEB128({(uint8_t)(-0x20)}, 7) << endl;
  cout << "void: " << hex << decodeSLEB128({(uint8_t)(-0x40)}, 7) << endl;
  uint32_t machine_code = 0xEA7D059B; // Example machine code for "mov x0, #1"
  // encodeAdr(0, 12);
}
