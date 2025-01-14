#pragma once
#include "OverloadOperator.h"
#include "Utils.h"
using namespace std;

class WasmType {
public:
  void print_data(TypeCategory category) {
    cout << "--- Printing " + type_category_to_string(category) + " data---" << endl;
    vector<wasm_type> *v = nullptr;
    if (category == TypeCategory::PARAM) {
      v = &param_data;
    } else if (category == TypeCategory::RESULT) {
      v = &result_data;
    }
    for (auto &elem : *v) {
      std::visit(
          [](auto &&value) {
            // value 的类型会被自动推断为四种之一
            std::cout << value << " is type: " << typeid(value).name() << std::endl;
          },
          elem);
    }
  }
  
  void add_data(TypeCategory category, const std::string &type) {
    wasm_type data;
    if (type == "7f") {
      data = static_cast<int32_t>(0);
    } else if (type == "7e") {
      data = static_cast<int64_t>(0);
    } else if (type == "7d") {
      data = static_cast<float>(0);
    } else if (type == "7c") {
      data = static_cast<double>(0);
    } else {
      std::cerr << "Adding data failed. Unknown data type: " << type << std::endl;
      return;
    }
    if (category == TypeCategory::PARAM) {
      param_data.push_back(data);
    } else if (category == TypeCategory::RESULT) {
      result_data.push_back(data);
    } 
  }

  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
};