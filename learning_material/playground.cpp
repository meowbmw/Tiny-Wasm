#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>

#include "../include/utils.h"

using namespace std;
int main() {
  string s = "e0 0f 00 b9 ";
  string bits = processHexCode(s, false);
  cout << "str w0, [sp, #12]" << endl;
  cout << bits << endl;
  return 0;
  // Extracting values
  unsigned int Rd = GetBits(bits, 0, 4);
  unsigned int imm12 = GetBits(bits, 10, 21);
  unsigned int Rn = GetBits(bits, 5, 9);
  unsigned int sf = GetBits(bits, 31, 31);
  unsigned int op = GetBits(bits, 30, 30);
  unsigned int S = GetBits(bits, 29, 29);
  unsigned int shift = GetBits(bits, 22, 23);

  // Output the extracted values
  std::cout << "Rd: " << Rd << std::endl;
  std::cout << "imm12: " << imm12 << std::endl;
  std::cout << "Rn: " << Rn << std::endl;
  std::cout << "sf: " << sf << std::endl;
  std::cout << "op: " << op << std::endl;
  std::cout << "S: " << S << std::endl;
  std::cout << "shift: " << shift << std::endl;
}