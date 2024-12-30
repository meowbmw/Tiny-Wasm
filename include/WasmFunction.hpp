#include "Opcode.hpp"
#include "container_print.h"
#include "float_utils.h"
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
    print_data(TypeCategory::PARAM);
    print_data(TypeCategory::LOCAL);
    getStackPreallocateSize();
    prepareStack();
    initParam();
    initLocal();
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
  }
  void allocateVar(const wasm_type &elem, int &stack_location) {
    std::visit(
        [&stack_location](auto &&value) {
          // value 的类型会被自动推断为四种之一
          char typeInfo = typeid(value).name()[0];
          if (typeInfo == 'f') {
            stack_location += 4;
          } else if (typeInfo == 'd') {
            stack_location += 8;
          } else if (typeInfo == 'l') {
            stack_location += 4;
          } else if (typeInfo == 'i') {
            stack_location += 4;
          }
        },
        elem);
  }
  void getStackPreallocateSize() {
    /**
     * calculate how much size should be allocated for stack
     */
    param_stack_start_location = 0;
    param_stack_end_location = param_stack_start_location;
    for (const auto &c : param_data) {
      allocateVar(c, param_stack_end_location);
    }
    local_stack_start_location = param_stack_end_location + 8;
    local_stack_end_location = local_stack_start_location;
    for (const auto &c : local_data) {
      allocateVar(c, local_stack_end_location);
    }
    // align to neaest 16 byte
    if (local_stack_end_location % 16 != 0) {
      stack_size = 16 * (local_stack_end_location / 16 + 1);
    } else {
      stack_size = local_stack_end_location;
    }
    cout << "Param start location: " << param_stack_start_location << endl;
    cout << "Param end location: " << param_stack_end_location << endl;
    cout << "Local start location: " << local_stack_start_location << endl;
    cout << "Local end location: " << local_stack_end_location << endl;
    cout << "Stack allocate size estimated to be: " << stack_size << endl;
  }
  void prepareStack() {
    string instr = toHexString(encodeAddSubImm(true, 31, 31, stack_size, false)).substr(2); // sub sp, sp, stack_size, substr to remove 0x prefix
    cout << format("Emit: sub sp, sp, {} | {}", stack_size, convertEndian(instr)) << endl;
    constructFullinstr(instr);
  }
  void initParam() {
    /**
     * Todo: only support 8 params for now!! need to support load from stack if we want to support more params
     */
    string instr;
    int offset = param_stack_end_location;
    int fp_reg_used = 0;
    int general_reg_used = 0;
    for (int i = 0; i < param_data.size(); ++i) {
      std::visit(
          [&offset, &general_reg_used, &fp_reg_used, &instr](auto &&value) {
            // value 的类型会被自动推断为四种之一
            char typeInfo = typeid(value).name()[0];
            if (typeInfo == 'f') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_F32, fp_reg_used, 31, offset >> 2)).substr(2);
              cout << format("Emit: str s{}, [sp, #{}] | {}", fp_reg_used, offset, convertEndian(instr)) << endl;
              offset -= 4;
              fp_reg_used += 1;
            } else if (typeInfo == 'd') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_F64, fp_reg_used, 31, offset >> 3)).substr(2);
              cout << format("Emit: str d{}, [sp, #{}] | {}", fp_reg_used, offset, convertEndian(instr)) << endl;
              offset -= 8;
              fp_reg_used += 1;
            } else if (typeInfo == 'l') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_64, general_reg_used, 31, offset >> 3)).substr(2);
              cout << format("Emit: str x{}, [sp, #{}] | {}", general_reg_used, offset, convertEndian(instr)) << endl;
              offset -= 4;
              general_reg_used += 1;
            } else if (typeInfo == 'i') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_32, general_reg_used, 31, offset >> 2)).substr(2);
              cout << format("Emit: str w{}, [sp, #{}] | {}", general_reg_used, offset, convertEndian(instr)) << endl;
              offset -= 4;
              general_reg_used += 1;
            }
          },
          param_data[i]);
      constructFullinstr(instr);
    }
  }
  void initLocal() {
    string instr;
    int offset = local_stack_end_location + 8; // leave some space between param and local
    for (int i = 0; i < local_data.size(); ++i) {
      std::visit(
          [&offset, &instr](auto &&value) {
            // value 的类型会被自动推断为四种之一
            char typeInfo = typeid(value).name()[0];
            if (typeInfo == 'f') {
              // NOTE: we are only moving zeros, so treat them like int here
              // xzr/wzr have same number as sp (31)
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_32, 31, 31, offset >> 2)).substr(2);
              cout << format("Emit: str wzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 4;
            } else if (typeInfo == 'd') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_64, 31, 31, offset >> 3)).substr(2);
              cout << format("Emit: str xzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 8;
            } else if (typeInfo == 'l') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_64, 31, 31, offset >> 3)).substr(2);
              cout << format("Emit: str xzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 4;
            } else if (typeInfo == 'i') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_32, 31, 31, offset >> 2)).substr(2);
              cout << format("Emit: str wzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 4;
            }
          },
          local_data[i]);
      constructFullinstr(instr);
    }
  }
  void restoreStack() {
    const string instr =
        toHexString(encodeAddSubImm(false, 31, 31, stack_size, false)).substr(2); // add sp, sp, stack_size, substr to remove 0x prefix
    cout << format("Emit: add sp, sp, {} | {}", stack_size, convertEndian(instr)) << endl;
    constructFullinstr(instr);
  }
  void emitRet() {
    const string instr = "C0035FD6";
    cout << format("Emit: ret | {}", convertEndian(instr)) << endl;
    constructFullinstr(instr);
  }
  void constructFullinstr(string sub_instr) {
    instructions = instructions + sub_instr;
  }
  void prepareParams() {
    /**
     * Store params to their respective location before calling our function
     * param_data[0] -> x0
     * param_data[1] -> x1
     */
    cout << "---Loading params to their respective registers---" << endl;
    for (int i = 0; i < param_data.size(); ++i) {
      std::visit(
          [&i, this](auto &&value) {
            // value 的类型会被自动推断为四种之一
            char typeInfo = typeid(value).name()[0];
            if (typeInfo == 'f') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'd') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'l') {
              const string instr = toHexString(encodeMovz(i, value, X_REG, 0)).substr(2); // add sp, sp, stack_size, substr to remove 0x prefix
              cout << format("Emit: mov x{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            } else if (typeInfo == 'i') {
              const string instr = toHexString(encodeMovz(i, value, W_REG, 0)).substr(2); // add sp, sp, stack_size, substr to remove 0x prefix
              cout << format("Emit: mov s{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            }
          },
          param_data[i]);
    }
    cout << "---Loading parameters finished---" << endl;
  }
  void executeInstr() {
    /**
     * Allocate memory with execute permission
     * And load machine code into that
     * Save address pointer to self.instructions
     */
    // Warn: Append pre instructions here
    // Also add branch instruction here; we might need to support function call later
    string branch_instr = toHexString(encodeBranch(1)).substr(2); // function will be right next to b instruction, so offset is 1 here
    instructions = pre_instructions_for_param_loading + branch_instr + instructions;
    cout << "Machine instruction to load: " << instructions << endl;
    const size_t arraySize = instructions.length() / 2;
    auto charArray = make_unique<unsigned char[]>(arraySize); // use smart pointer here so we don't need to free it manually
    for (size_t i = 0; i < arraySize; i++) {
      const string byteStr = instructions.substr(i * 2, 2);
      charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    int64_t (*instruction_set)() = nullptr;
    // allocate executable buffer
    instruction_set =
        reinterpret_cast<int64_t (*)()>(mmap(nullptr, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (instruction_set == MAP_FAILED) {
      perror("mmap");
      return;
    }
    // copy code to buffer
    memcpy(reinterpret_cast<void *>(instruction_set), charArray.get(), arraySize);
    // ensure memcpy isn't optimized away as a dead store.
    __builtin___clear_cache(reinterpret_cast<char *>(instruction_set), reinterpret_cast<char *>(instruction_set) + arraySize);
    // !不需要做任何传参，因为参数已经放在寄存器里啦
    int64_t ans = instruction_set();
    munmap(reinterpret_cast<void *>(instruction_set), arraySize);
  }
  void print_data(TypeCategory category) {
    cout << "--- Printing " + type_category_to_string(category) + " data---" << endl;
    bool empty_flag = false;
    vector<wasm_type> *v = nullptr;
    if (category == TypeCategory::LOCAL) {
      v = &local_data;
      if (local_data.size() == 0) {
        empty_flag = true;
      }
    } else if (category == TypeCategory::PARAM) {
      v = &param_data;
      if (param_data.size() == 0) {
        empty_flag = true;
      }
    } else if (category == TypeCategory::RESULT) {
      v = &result_data;
      if (result_data.size() == 0) {
        empty_flag = true;
      }
    }
    if (empty_flag == true) {
      cout << "Empty! Nothing here" << endl;
      return;
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
  string pre_instructions_for_param_loading;
  vector<wasm_type> local_data;
  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
  vector<wasm_type> stack;
  map<pair<TypeCategory, int>, int> var_stack_locater;
  map<int, pair<TypeCategory, int>> var_stack_getter;
  int stack_size = 0;
  int param_stack_start_location = 0;
  int param_stack_end_location = 0;
  int local_stack_start_location = 0;
  int local_stack_end_location = 0;
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