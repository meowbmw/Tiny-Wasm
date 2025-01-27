#pragma once
#include "../FloatUtils.h"
#include "../Opcode.hpp"
#include "../OverloadOperator.h"
#include "../Utils.h"
using namespace std;
using ArithOperation = std::function<wasm_type(wasm_type, wasm_type)>;

struct controlFlowElement {
  controlFlowElement(string s, int cur_length, vector<wasm_type> v) {
    label = s;
    current_wasm_length = cur_length;
    signature = v;
  }
  string label;
  int current_wasm_length;
  vector<wasm_type> signature;
};

string getSetJmpInstr() {
  /**
   * Warn: Use std::call_once and std::once_flag to make sure getSetJmpInstr is not evaluated repeatedly, it only needs to be excuted once.
   *  	stp	x19, x20, [x0, 0]
        stp	x21, x22, [x0, 2<<3]
        stp	x23, x24, [x0, 4<<3]
        stp	x25, x26, [x0, 6<<3]
        stp	x27, x28, [x0, 8<<3]
        stp	x29, x30, [x0, 10<<3]
        mov	x2,  sp
        str	x2,  [x0, 13<<3]
        mov	w0, #0
        ret
   */
  static std::once_flag flag;
  static std::string instr;
  std::call_once(flag, []() {
    LdStType ldstType = LdStType::STR;
    instr += encodeLdpStp(X_REG, ldstType, 19, 20, 0, 0);
    instr += encodeLdpStp(X_REG, ldstType, 21, 22, 0, 2 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 23, 24, 0, 4 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 25, 26, 0, 6 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 27, 28, 0, 8 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 29, 30, 0, 10 << 3);
    instr += encodeMovSP(X_REG, 2, 31);
    instr += encodeLoadStoreImm(X_REG, ldstType, 2, 0, 13 << 3);
    instr += encodeMovz(0, 0, W_REG, 0);
    instr += encodeReturn();
  });
  // cout << "Function SetJmp: " << instr << endl;
  return instr;
}
string getLongJmpInstr() {
  /**
   * Warn: Use std::call_once and std::once_flag to make sure getLongJmpInstr is not evaluated repeatedly, it only needs to be excuted once.
   *  ldp	x19, x20, [x0, 0<<3]
      ldp	x21, x22, [x0, 2<<3]
      ldp	x23, x24, [x0, 4<<3]
      ldp	x25, x26, [x0, 6<<3]
      ldp	x27, x28, [x0, 8<<3]
      ldp	x29, x30, [x0, 10<<3]
      ldr	x5, [x0, 13<<3]; x5 <- [x0, 104]
      mov	sp, x5; sp <- x5
      cmp	x1, #0
      mov	x0, #1
      csel	x0, x1, x0, ne
      br	x30
   */
  static std::once_flag flag;
  static std::string instr;
  std::call_once(flag, []() {
    LdStType ldstType = LdStType::LDR;
    instr += encodeLdpStp(X_REG, ldstType, 19, 20, 0, 0 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 21, 22, 0, 2 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 23, 24, 0, 4 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 25, 26, 0, 6 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 27, 28, 0, 8 << 3);
    instr += encodeLdpStp(X_REG, ldstType, 29, 30, 0, 10 << 3);
    instr += encodeLoadStoreImm(X_REG, ldstType, 5, 0, 13 << 3);
    instr += encodeMovSP(X_REG, 31, 5);
    instr += encodeCompareImm(X_REG, 1, 0);
    instr += encodeMovz(0, 1, X_REG, 0);
    instr += encodeCSEL(X_REG, 0, 1, 0, reverse_cond_str_map.at("ne"));
    instr += encodeBranchRegister(30);
  });
  // cout << "Function LongJmp: " << instr << endl;
  return instr;
}
