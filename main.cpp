#include "include/Json.hpp"
#include "include/Parser.hpp"
using namespace std;
using json = nlohmann::json;

// aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
// aarch64-linux-gnu-g++ parser.cpp -o main && qemu-aarch64 -L /usr/aarch64-linux-gnu ./main
// qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu ./main
int main() {
  //   Parser parser("test/local.2.wasm");
  //   parser.parse();
  //   parser.funcBatchProcess(true);
  //   return 0;
  ofstream parser_cout("parserOutput.txt");
  auto normal_cout = cout.rdbuf();
  string base_path = "test/CH03/";
  //   ifstream f(base_path + "custom.json");
  ifstream f(base_path + "arithmetic.json");
  json data = json::parse(f);
  multimap<string, json> command_map;
  map<string, Parser> parser_map;
  string cur_wasm_file;
  for (size_t i = 0; i < data["commands"].size(); ++i) {
    if (data["commands"][i].contains("filename")) {
      cur_wasm_file = data["commands"][i]["filename"];
      if (parser_map.contains(cur_wasm_file) == false) {
        Parser cur_parser = Parser(base_path + cur_wasm_file);
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
        param_data[i] = static_cast<int32_t>(stoul(v_str));
      } else if (v.second["action"]["args"][i]["type"] == "i64") {
        param_data[i] = static_cast<int64_t>(stoul(v_str));
      } else {
        cout << "Unsupported param type, probably float" << endl;
      }
    }
    cout << "param_data: " << param_data << endl;
    cout.rdbuf(parser_cout.rdbuf()); // Redirect parser output to file; it's too much...
    curParser.funcSingleProcess(function_index);
    cout.rdbuf(normal_cout); // Restore cout
    cout << v.first << " " << v.second << endl;
    string expect_str = v.second["expected"][0]["value"].dump();
    expect_str = expect_str.substr(1, expect_str.size() - 2);
    cout << "Executing function " << function_index << ": " << function_name << endl;
    if (curFunction.result_data.size() > 0) {
      auto ans = curFunction.executeInstr();
      cout << format("Expecting {}", expect_str) << endl;
      cout << "Result: " << ans << endl;
      bool matched = (ans == static_cast<int64_t>(stoul(expect_str)));
      cout << "Matched: " << (matched ? "True" : "False") << endl;
      if (matched == false) {
        throw "Unmatched";
      }
    } else {
      cout << "[]" << endl;
      cout << format("Expecting []") << endl;
      cout << "Matched: " << ((expect_str == "ul") ? "True" : "False") << endl;
    }
  }
  return 0;
}