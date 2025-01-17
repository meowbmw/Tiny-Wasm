#include "include/Parser.hpp"
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

// aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
// aarch64-linux-gnu-g++ main.cpp -o main && qemu-aarch64 -L /usr/aarch64-linux-gnu ./main
// ccache /usr/bin/clang++ --target=aarch64-linux-gnu -std=c++20 -g main.cpp -o main -lcapstone
void test_chapter(const string &chapter_number) {
  ofstream parser_cout("parserOutput.txt");
  auto normal_cout = cout.rdbuf();
  ifstream f;
  string base_path = format("test/CH{}/", chapter_number);
  if (chapter_number == "02") {
    f = ifstream(base_path + "local.json");
  } else if (chapter_number == "03") {
    f = ifstream(base_path + "arithmetic.json");
    // f = ifstream(base_path + "custom.json");
  } else if (chapter_number == "04") {
    f = ifstream(base_path + "div.json");
    // f = ifstream(base_path + "custom.json");
  }
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
        cout << endl;
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
    bool matched = false;
    bool exceptionThrown = false;
    int64_t ans;
    try {
      ans = curFunction.executeWasmInstr();
    } catch (string s) {
      exceptionThrown = true;
    }
    if (v.second["type"] == "assert_trap") {
      cout << format("Expecting: {}", "trap") << endl;
      cout << "Result: " << ((exceptionThrown) ? "trap" : to_string(ans)) << endl;
      matched = exceptionThrown;
      cout << "Matched: " << (matched ? "True" : "False") << endl;
    } else if (curFunction.result_data.size() > 0) {
      cout << format("Expecting: {}", expect_str) << endl;
      cout << "Result: " << ans << endl;
      matched = (ans == static_cast<int64_t>(stoul(expect_str)));
      cout << "Matched: " << (matched ? "True" : "False") << endl;
    } else {
      cout << format("Expecting: []") << endl;
      cout << "Result: []" << endl;
      matched = (expect_str == "ul");
      cout << "Matched: " << (matched ? "True" : "False") << endl;
    }
    if (matched == false) {
      throw "Unmatched";
    }
  }
}
int main() {
  vector<string> test_chapters = {"02", "03"};

  // vector<string> test_chapters = {"04"};
  cout << "A simple testing program to check our JIT works as intended." << endl;
  cout << "Chapters to test: " << test_chapters << endl;
  for (auto &chapter_number : test_chapters) {
    cout << "--- Testing chapter " << chapter_number << " ---" << endl;
    test_chapter(chapter_number);
    cout << endl;
  }
  return 0;
}