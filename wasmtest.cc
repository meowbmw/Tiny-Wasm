#include <gtest/gtest.h>

#include "include/WasmFile.hpp"
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

void test_chapter(string chapter_number, string test_json) {
  ofstream parser_cout(format("Testing/parserOutput{}.txt", chapter_number));
  ofstream normal_cout(format("Testing/testOutput{}.txt", chapter_number));
  // auto normal_cout = cout.rdbuf();
  string base_path = format("../test/CH{}/", chapter_number);
  ifstream f(base_path + test_json);
  json data = json::parse(f);
  multimap<string, json> command_map;
  map<string, WasmFile> wasmFile_map;
  string cur_wasm_file;
  for (size_t i = 0; i < data["commands"].size(); ++i) {
    if (data["commands"][i].contains("filename")) {
      cur_wasm_file = data["commands"][i]["filename"];
      if (wasmFile_map.contains(cur_wasm_file) == false) {
        WasmFile cur_wasmFile = WasmFile(base_path + cur_wasm_file);
        wasmFile_map.insert({cur_wasm_file, std::move(cur_wasmFile)});
        cout.rdbuf(parser_cout.rdbuf()); // Redirect parser output to file; it's too much...
        wasmFile_map[cur_wasm_file].parse();
        cout.rdbuf(0);
        wasmFile_map[cur_wasm_file].funcBatchProcess(); // do whole init now
        cout.rdbuf(normal_cout.rdbuf());                // Restore cout
      }
    } else if (data["commands"][i].contains("action")) {
      command_map.insert({cur_wasm_file, data["commands"][i]});
    }
  }
  for (auto &v : command_map) {
    cout << "---Asserting---" << endl;
    string function_name = v.second["action"]["field"];
    WasmFile &curParser = wasmFile_map[v.first];
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
    curParser.wasmFunctionVec[function_index].clear(); // TODO: this could be optimized, no need to clear and initialize again; but currently it will
    cout.rdbuf(parser_cout.rdbuf());                   // Redirect parser output to file; it's too much...
    curParser.funcSingleProcess(function_index);
    cout.rdbuf(normal_cout.rdbuf()); // Restore cout
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
    EXPECT_EQ(matched, true);
  }
}

TEST(WASM_TEST, CH02) {
  test_chapter("02", "local.json");
}

TEST(WASM_TEST, CH03) {
  test_chapter("03", "arithmetic.json");
}

TEST(WASM_TEST, CH04) {
  test_chapter("04", "div.json");
}

TEST(WASM_TEST, CH05) {
  test_chapter("05", "if.json");
}

TEST(WASM_TEST, CH06) {
  test_chapter("06", "block.json");
}

TEST(WASM_TEST, CH07) {
  test_chapter("07", "loop.json");
}

TEST(WASM_TEST, CH08) {
  test_chapter("08", "call.json");
}

TEST(WASM_TEST, CH09) {
  test_chapter("09", "call_indirect.json");
}

TEST(WASM_TEST, CH10) {
  test_chapter("10", "global.json");
}

TEST(WASM_TEST, CH11) {
  test_chapter("11", "data.json");
  test_chapter("11", "store.json");
  test_chapter("11", "load.json");
  test_chapter("11", "grow.json");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::GTEST_FLAG(break_on_failure) = true; // NOTE: stop immediately when one assertion has failed
  // 动态生成过滤器字符串
  ostringstream filter_stream;
  ostringstream test_name;
  test_name << "WASM_TEST.CH" << std::setw(2) << std::setfill('0') << argv[1];
  filter_stream << test_name.str();
  ::testing::GTEST_FLAG(filter) = filter_stream.str();
  return RUN_ALL_TESTS();
}