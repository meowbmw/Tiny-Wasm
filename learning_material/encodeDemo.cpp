#include "../include/WasmFunction.hpp"
int main() {
 string instr;
    instr += encodeLdpStp(X_REG, STR, 29, 30, 31, -16, EncodingMode::PreIndex);
    instr += encodeMovSP(X_REG, 29, 31);
    cout << endl;
    instr += encodeLdpStp(X_REG, STR, 29, 30, 31, 16, EncodingMode::PostIndex);
    instr += encodeReturn();
}