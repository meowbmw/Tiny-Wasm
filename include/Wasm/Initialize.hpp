#pragma once
#include "WasmFunction.hpp"

void WasmFunction::initParam() {
  /**
   * Todo: only support 8 params for now!! need to support load from stack if we want to support more params
   */
  string instr;
  int offset = param_stack_end_location;
  int fp_reg_used = 0;
  int general_reg_used = 0;
  for (int i = 0; i < param_data.size(); ++i) {
    std::visit(
        [&offset, &general_reg_used, &fp_reg_used, &instr, &i, this](auto &&value) {
          char typeInfo = typeid(value).name()[0];
          vecToStack[{TypeCategory::PARAM, i}] = offset;
          stackToVec[offset] = {TypeCategory::PARAM, i};
          if (typeInfo == 'f') {
            instr = encodeLoadStoreImm(S_REG, STR, fp_reg_used, 31, offset);
            regTypeGetter[{TypeCategory::PARAM, i}] = S_REG;
            offset -= 4;
            fp_reg_used += 1;
          } else if (typeInfo == 'd') {
            instr = encodeLoadStoreImm(D_REG, STR, fp_reg_used, 31, offset);
            regTypeGetter[{TypeCategory::PARAM, i}] = D_REG;
            offset -= 8;
            fp_reg_used += 1;
          } else if (typeInfo == 'l') {
            instr = encodeLoadStoreImm(X_REG, STR, general_reg_used, 31, offset);
            regTypeGetter[{TypeCategory::PARAM, i}] = X_REG;
            offset -= 4;
            general_reg_used += 1;
          } else if (typeInfo == 'i') {
            instr = encodeLoadStoreImm(W_REG, STR, general_reg_used, 31, offset);
            regTypeGetter[{TypeCategory::PARAM, i}] = W_REG;
            offset -= 4;
            general_reg_used += 1;
          }
        },
        param_data[i]);
    constructFullinstr(instr);
  }
}
void WasmFunction::initLocal() {
  string instr;
  int offset = local_stack_end_location;
  for (int i = 0; i < local_data.size(); ++i) {
    std::visit(
        [&offset, &instr, &i, this](auto &&value) {
          char typeInfo = typeid(value).name()[0];
          vecToStack[{TypeCategory::LOCAL, i}] = offset;
          stackToVec[offset] = {TypeCategory::LOCAL, i};
          if (typeInfo == 'f') {
            // NOTE: we are only moving zeros, so treat them like int here
            // xzr/wzr have same number as sp (31)
            instr = encodeLoadStoreImm(W_REG, STR, 31, 31, offset);
            regTypeGetter[{TypeCategory::LOCAL, i}] = S_REG;
            offset -= 4;
          } else if (typeInfo == 'd') {
            instr = encodeLoadStoreImm(X_REG, STR, 31, 31, offset);
            regTypeGetter[{TypeCategory::LOCAL, i}] = D_REG;
            offset -= 8;
          } else if (typeInfo == 'l') {
            instr = encodeLoadStoreImm(X_REG, STR, 31, 31, offset);
            regTypeGetter[{TypeCategory::LOCAL, i}] = X_REG;
            offset -= 4;
          } else if (typeInfo == 'i') {
            instr = encodeLoadStoreImm(W_REG, STR, 31, 31, offset);
            regTypeGetter[{TypeCategory::LOCAL, i}] = W_REG;
            offset -= 4;
          }
        },
        local_data[i]);
    constructFullinstr(instr);
  }
}
