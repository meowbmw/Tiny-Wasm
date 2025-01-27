#pragma once
#include "WasmFunction.hpp"
void WasmFunction::printOriginWasmOpcode(int &offset) {
    cout << "Origin WASM Opcode: ";
    for (int i = offset; i < code_vec.size(); ++i) {
      cout << code_vec[i] << " ";
    }
    cout << endl;
  }
  
void WasmFunction::print_data(TypeCategory category) {
  cout << "--- Printing " + type_category_to_string(category) + " data---" << endl;
  bool empty_flag = false;
  vector<wasm_type> v;
  if (category == TypeCategory::LOCAL) {
    v = local_data;
    if (local_data.size() == 0) {
      empty_flag = true;
    }
  } else if (category == TypeCategory::PARAM) {
    v = param_data;
    if (param_data.size() == 0) {
      empty_flag = true;
    }
  } else if (category == TypeCategory::RESULT) {
    v = result_data;
    if (result_data.size() == 0) {
      empty_flag = true;
    }
  }
  if (empty_flag == true) {
    cout << "Empty! Nothing here" << endl;
    return;
  }
  for (size_t i = 0; i < v.size(); ++i) {
    std::visit(
        [&category, &i](auto &&value) {
          // value 的类型会被自动推断为四种之一
          cout << format("{}[{}]: ({}) = {}", type_category_to_string(category), i, typeid(value).name(), value) << endl;
        },
        v[i]);
  }
}
void WasmFunction::print_stack() {
  cout << "--- Printing stack (This is not wasm stack!) ---" << endl;
  if (stack.size() == 0) {
    cout << "Empty stack!" << endl;
  }
  for (size_t i = 0; i < stack.size(); ++i) {
    std::visit(
        [&i](auto &&value) {
          cout << format("stack[{}]: ({}) = {}", i, typeid(value).name(), value) << endl;
        },
        stack[i]);
  }
}
