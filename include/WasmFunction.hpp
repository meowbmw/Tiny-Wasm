#include "container_print.h"
#include "utils.h"
using namespace std;

class WasmFunction {
public:
  void processCodeVec() {
    int offset = 0;
    for (int i = 0; i < local_var_declare_count; ++i) {
      const unsigned int var_count_in_this_declare = stoul(code_vec[offset], nullptr, 16);
      const string var_type_in_this_declare = code_vec[offset + 1];
      for (int j = 0; j < var_count_in_this_declare; ++j) {
        add_local(var_type_in_this_declare);
      }
      offset += 2;
    }
    cout << "Origin WASM Opcode: ";
    for (int i = offset; i < code_vec.size(); ++i) {
      cout << code_vec[i] << " ";
    }
    cout << endl;
    int i = offset;
    prepareStack();
    while (i < code_vec.size()) {
      if (code_vec[i] == "0f") { // ret
        restoreStack();
        emitRet();
        ++i;
      } else if (code_vec[i] == "0b") { // end
        restoreStack();
        emitRet();
        ++i;
      } else if (code_vec[i] == "20") { // local.get
        const u_int64_t var_to_get = stoul(code_vec[i + 1], nullptr, 16);
        if (var_to_get < param_data.size() + local_data.size()) {
          if (var_to_get < param_data.size()) {
            // falls within boundry of param variable
            stack.push_back(param_data[var_to_get]);
          } else {
            // must be local variable then.
            stack.push_back(local_data[var_to_get - param_data.size()]);
          }
          i += 2;
        } else {
          cout << "Too big index {" + to_string(var_to_get) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
      } else if (code_vec[i] == "21") { // local.set
        const u_int64_t var_to_set = stoul(code_vec[i + 1], nullptr, 16);
        if (var_to_set < param_data.size() + local_data.size()) {
          if (var_to_set < param_data.size()) {
            param_data[var_to_set] = stack.back();
          } else {
            local_data[var_to_set - param_data.size()] = stack.back();
          }
          stack.pop_back();
          i += 2;
        } else {
          cout << "Too big index {" + to_string(var_to_set) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
      } else if (code_vec[i] == "22") { // local.tee
        const u_int64_t var_to_set = stoul(code_vec[i + 1], nullptr, 16);
        if (var_to_set < param_data.size() + local_data.size()) {
          if (var_to_set < param_data.size()) {
            param_data[var_to_set] = stack.back();
          } else {
            local_data[var_to_set - param_data.size()] = stack.back();
          }
          i += 2;
        } else {
          cout << "Too big index {" + to_string(var_to_set) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
      } else if (code_vec[i] == "41") { // i32.const
        stack.push_back(static_cast<int32_t>(stoul(code_vec[i + 1], nullptr, 16)));
        i += 2;
      } else if (code_vec[i] == "42") { // i64.const
        stack.push_back(static_cast<int64_t>(stoul(code_vec[i + 1], nullptr, 16)));
        i += 2;
      } else if (code_vec[i] == "43") { // f32.const
        stack.push_back(hexToFloat(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4]));
        i += 5;
      } else if (code_vec[i] == "44") { // f64.const
        stack.push_back(
            hexToDouble(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4] + code_vec[i + 5] + code_vec[i + 6] + code_vec[i + 7]));
        i += 9;
      }
    }
    // print_data(TypeCategory::LOCAL);
    // print_data(TypeCategory::PARAM);
    // executeInstr();
    // print_stack();
  }
  void prepareStack(){
    const string instr = "ff8304d1"; // sub sp, sp, #288, todo: hardcoded stack size for now, 288 should be large enough
    construct_instr(instr);
  }
  void restoreStack(){
    const string instr = "ff830491"; //  add sp, sp, 288
    construct_instr(instr);
  }
  void emitRet() {
    // cout << "Emitting Return" << endl;
    const string instr = "c0035fd6";
    construct_instr(instr);
    // cout << "Executed" << endl;
  }
  void construct_instr(string sub_instr){
    instructions = instructions + sub_instr;
  }
  void executeInstr() {
    /**
     * Allocate memory with execute permission
     * And load machine code into that
     * Save address pointer to self.instructions
     */
    cout << "Machine instruction to load: " << instructions << endl;
    const size_t arraySize = instructions.length() / 2;
    auto charArray = make_unique<unsigned char[]>(arraySize); // use smart pointer here so we don't need to free it manually
    for (size_t i = 0; i < arraySize; i++) {
      const string byteStr = instructions.substr(i * 2, 2);
      charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    void (*instruction_set)() = nullptr;
    // allocate executable buffer
    instruction_set = reinterpret_cast<void (*)()>(mmap(nullptr, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (instruction_set == MAP_FAILED) {
      perror("mmap");
      return;
    }
    // copy code to buffer
    memcpy(reinterpret_cast<void *>(instruction_set), charArray.get(), arraySize);
    // ensure memcpy isn't optimized away as a dead store.
    __builtin___clear_cache(reinterpret_cast<char *>(instruction_set), reinterpret_cast<char *>(instruction_set) + arraySize);
    instruction_set();
    munmap(reinterpret_cast<void *>(instruction_set), arraySize);
  }
  void print_data(TypeCategory category) {
    cout << "--- Printing " + type_category_to_string(category) + " data---" << endl;
    vector<wasm_type> *v = nullptr;
    if (category == TypeCategory::LOCAL) {
      v = &local_data;
    }
    else if (category == TypeCategory::PARAM) {
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
  void print_stack() {
    cout << "--- Printing stack---" << endl;
    for (auto &elem : stack) {
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
    } else if (category == TypeCategory::LOCAL) {
      local_data.push_back(data);
    }
  }
  void add_param(const std::string &type) {
    add_data(TypeCategory::PARAM, type);
  }
  void add_result(const std::string &type) {
    add_data(TypeCategory::RESULT, type);
  }
  void add_local(const std::string &type) {
    add_data(TypeCategory::LOCAL, type);
  }
  void set_code_vec(vector<string> &v, size_t l = 0) {
    code_vec = v;
    local_var_declare_count = l;
  }
  vector<string> code_vec;
  u_int64_t local_var_declare_count = 0;
  string instructions;
  vector<wasm_type> local_data;
  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
  vector<wasm_type> stack;
  int type;
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