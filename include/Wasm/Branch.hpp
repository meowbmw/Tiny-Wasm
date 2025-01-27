#pragma once
#include "WasmFunction.hpp"

void WasmFunction::fakeInsertBranch(string label, string BranchStr) {
  int64_t cur_location = wasm_instructions.size();
  cout << format("       {} {}", BranchStr, label) << endl;
  wasm_instructions += "FFFFFFFF"; // fake insert branch instruction
  fake_insert_map.insert({label, {cur_location, BranchStr}});
}
void WasmFunction::insertLabel(string label) {
  cout << format("{}: ", label) << endl;
  label_map.insert({label, wasm_instructions.size()});
}
void WasmFunction::fixUpfakeBranch() {
  /**
   * After processing codevec finished, call this function to fix fake branch instructions
   * it will replace fake instruction with real ones
   */
  for (const auto &x : fake_insert_map) {
    auto [origin_location, BranchStr] = x.second;
    cout << "++++++++++++++++" << endl;
    cout << format("Fixing fake branch to {} at {}", x.first, origin_location) << endl;
    cout << "Before fix: " << wasm_instructions.substr(origin_location, 8) << endl;
    // cout << wasm_instructions << endl;
    int instructions_level_offset = (label_map[x.first] - origin_location) / 8;
    cout << "Instructions level offset (estimated): " << instructions_level_offset << endl;
    if (BranchStr == "b") {
      wasm_instructions.replace(origin_location, 8, encodeBranch(instructions_level_offset));
    } else if (BranchStr == "bl") {
      wasm_instructions.replace(origin_location, 8, encodeBranch(instructions_level_offset, true));
    } else {
      // be,beq,bne,etc..
      wasm_instructions.replace(origin_location, 8, encodeBranchCondition(instructions_level_offset, reverse_cond_str_map.at(BranchStr.substr(1))));
    }
    cout << "After fix: " << wasm_instructions.substr(origin_location, 8) << endl;
    // cout << wasm_instructions << endl;
  }
}
