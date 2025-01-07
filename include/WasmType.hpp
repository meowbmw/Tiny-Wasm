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
  /**
   * we have 4 vectors
   * vector<int>, vector<double> ..
   * we have 5 variables, 1xint,2xdouble,1xlong,1xint
   * index is: 00,01,02,03,04
   * we need to be able to access by index
   * when adding to a vector, remeber its current index
   * like, index in locals 04, corresponding vector index 01
   */
};