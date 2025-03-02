#include "include/WasmFile.hpp"
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

/**
 * Run command:
    aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
    aarch64-linux-gnu-g++ main.cpp -o main && qemu-aarch64 -L /usr/aarch64-linux-gnu ./main
    ccache /usr/bin/clang++ --target=aarch64-linux-gnu -std=c++20 -g main.cpp -o main -lcapstone
 * HINT: lldb read memory usage
 * read 4 bytes from [x20, x22]
 * memory read -f x -c 4 `$x20 + $x22`
 */
auto normal_cout = cout.rdbuf();

void test_chapter(const string &chapter_number) {
  ofstream parser_cout("parserOutput.txt");
  cout.rdbuf(parser_cout.rdbuf()); // Redirect parser output to file; it's too much...
  ifstream f;
  string base_path = format("test/CH{}/", chapter_number);
  if (chapter_number == "02") {
    f = ifstream(base_path + "local.json");
  } else if (chapter_number == "03") {
    f = ifstream(base_path + "arithmetic.json");
  } else if (chapter_number == "04") {
    f = ifstream(base_path + "div.json");
  } else if (chapter_number == "05") {
    f = ifstream(base_path + "if.json");
  } else if (chapter_number == "06") {
    f = ifstream(base_path + "block.json");
  } else if (chapter_number == "07") {
    f = ifstream(base_path + "loop.json");
  } else if (chapter_number == "08") {
    f = ifstream(base_path + "call.json");
  }
  json data = json::parse(f);
  multimap<string, json> command_map;
  map<string, WasmFile> wasmFile_map;
  string cur_wasm_file;
  for (size_t i = 0; i < data["commands"].size(); ++i) {
    if (data["commands"][i].contains("filename")) {
      cur_wasm_file = data["commands"][i]["filename"];
      if (wasmFile_map.contains(cur_wasm_file) == false) {
        WasmFile cur_wasmFile = WasmFile(base_path + cur_wasm_file);
        wasmFile_map.insert({cur_wasm_file, cur_wasmFile});
        wasmFile_map[cur_wasm_file].parse();
        wasmFile_map[cur_wasm_file].funcBatchProcess(); // do whole init now
        cout << endl;
      }
    } else if (data["commands"][i].contains("action")) {
      command_map.insert({cur_wasm_file, data["commands"][i]});
    }
  }
  for (auto &v : command_map) {
    cout << "=====================================================================" << endl;
    cout << v.first << " " << v.second << endl;
    cout << "------ Input ------" << endl;
    string function_name = v.second["action"]["field"];
    WasmFile &curParser = wasmFile_map[v.first];
    int function_index = curParser.funcNameIndexMapper[function_name];
    // curParser.initFunctionbyType(function_index);
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
        param_data[i] = static_cast<int64_t>(stoull(v_str));
      } else {
        cout << "Unsupported param type, probably float" << endl;
      }
    }
    cout << "param_data: " << param_data << endl;
    cout << endl;
    curParser.wasmFunctionVec[function_index].clear(); // TODO: this could be optimized, no need to clear and initialize again; but currently it will remain a simple hack to enable BatchProcess and SingleProcess to co-exist
    curParser.funcSingleProcess(function_index);
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
    cout << endl;
  }
}
int main() {
  // vector<string> test_chapters = {"02", "03", "04", "05", "06", "07"};

  vector<string> test_chapters = {"08"};
  cout << "A simple testing program to check our JIT works as intended." << endl;
  cout << "Chapters to test: " << test_chapters << endl;
  for (auto &chapter_number : test_chapters) {
    cout << "--- Testing chapter " << chapter_number << " ---" << endl;
    test_chapter(chapter_number);
    cout.rdbuf(normal_cout); // Restore cout
    cout << "Ok" << endl;
  }
  return 0;
}