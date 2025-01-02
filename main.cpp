#include "include/parser.hpp"
#include "include/json.hpp"
using namespace std;
using json = nlohmann::json;

// aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
// aarch64-linux-gnu-g++ parser.cpp -o main && qemu-aarch64 -L /usr/aarch64-linux-gnu ./main
// qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu ./main

int main() {
  Parser parser;
  parser.parse(WASM_TO_READ);
  parser.funcBatchProcess(true);
}