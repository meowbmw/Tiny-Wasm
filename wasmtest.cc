#include <gtest/gtest.h>

#include "include/json.hpp"
#include "include/parser.hpp"
using namespace std;
using json = nlohmann::json;

TEST(WASM_TEST, wasm_test) {
  ofstream parser_cout("parserOutput.txt");
  auto normal_cout = cout.rdbuf();

  ifstream f("../test/local.json");
  json data = json::parse(f);
  multimap<string, json> command_map;
  map<string, Parser> parser_map;
  string cur_wasm_file;
  for (size_t i = 0; i < data["commands"].size(); ++i) {
    if (data["commands"][i].contains("filename")) {
      cur_wasm_file = data["commands"][i]["filename"];
      if (parser_map.contains(cur_wasm_file) == false) {
        Parser cur_parser = Parser("../test/" + cur_wasm_file);
        parser_map.insert({cur_wasm_file, cur_parser});
        cout.rdbuf(parser_cout.rdbuf()); // Redirect parser output to file; it's too much...
        parser_map[cur_wasm_file].parse();
        cout.rdbuf(normal_cout); // Restore cout
      }
    } else if (data["commands"][i].contains("action")) {
      command_map.insert({cur_wasm_file, data["commands"][i]});
    }
  }
  for (auto &v : command_map) {
    cout << "---Asserting---" << endl;
    string function_name = v.second["action"]["field"];
    Parser &curParser = parser_map[v.first];
    int function_index = curParser.funcNameIndexMapper[function_name];
    curParser.initFunctionbyType(function_index);
    // NOTE: USING REFERENCE IS VERY VERY IMPORTANT HERE!!!
    // OTHERWISE ORIGIN VALUE WON'T BE CHANGED!!
    auto &curFunction = curParser.wasmFunctionVec[function_index];
    auto &param_data = curFunction.param_data;
    for (int i = 0; i < param_data.size(); ++i) {
      auto v_str = v.second["action"]["args"][i]["value"].dump();
      v_str = v_str.substr(1, v_str.size() - 2);
      if (v.second["action"]["args"][i]["type"] == "i32") {
        param_data[i] = stoi(v_str);
      } else if (v.second["action"]["args"][i]["type"] == "i64") {
        param_data[i] = stol(v_str);
      } else {
        cout << "Unsupported param type, probably float" << endl;
      }
    }
    cout.rdbuf(parser_cout.rdbuf()); // Redirect parser output to file; it's too much...
    curParser.funcSingleProcess(function_index);
    cout.rdbuf(normal_cout); // Restore cout
    cout << "Module: " << v.first << ", Command: " << v.second << endl;
    cout << "Param data: " << param_data << endl;
    string expect_str = v.second["expected"][0]["value"].dump();
    expect_str = expect_str.substr(1, expect_str.size() - 2);
    cout << "Executing function " << function_index << ": " << function_name << endl;
    if (curFunction.result_data.size() > 0) {
      auto ans = curFunction.executeInstr();
      cout << format("Expecting {}", expect_str) << endl;
      cout << "Result: " << ans << endl;
      EXPECT_EQ(ans, stol(expect_str));
    } else {
      cout << "Result: []" << endl;
      cout << format("Expecting []") << endl;
      EXPECT_EQ(expect_str, "ul");
    }
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}