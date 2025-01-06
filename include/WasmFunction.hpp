#pragma once
#include "FloatUtils.h"
#include "Opcode.hpp"
#include "OverloadOperator.h"
#include "Utils.h"
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
    print_data(TypeCategory::PARAM);
    print_data(TypeCategory::LOCAL);
    getStackPreallocateSize(offset);
    prepareStack();
    initParam(); // initParam is storing to memory, prepareParams is storing to registers
    initLocal();
    printInitStack();
    runningWasmCode(offset);
    print_stack();
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
  void getStackPreallocateSize(const int offset) {
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
    cout << "--- Estimate stack allocation ---" << endl;
    cout << "Param start location: " << param_stack_start_location << endl;
    cout << "Param end location: " << param_stack_end_location << endl;
    cout << "Local start location: " << local_stack_start_location << endl;
    cout << "Local end location: " << local_stack_end_location << endl;
    wasm_stack_start_location = local_stack_end_location + 8;
    int wasm_stack_size = (code_vec.size() - offset) * 4;
    wasm_stack_end_location = wasm_stack_start_location + wasm_stack_size;
    cout << "Wasm stack start location: " << wasm_stack_start_location << endl;
    cout << format("Adding maximum possible wasm stack size: (code_vec.size: {} - offset: {}) * 4 = {}", code_vec.size(), offset, wasm_stack_size)
         << endl;
    cout << "Wasm stack end location: " << wasm_stack_end_location << endl;
    // align to neaest 16 byte
    if (wasm_stack_end_location % 16 != 0) {
      stack_size = 16 * (wasm_stack_end_location / 16 + 1);
    } else {
      stack_size = wasm_stack_end_location;
    }
    cout << "Stack allocate size estimated to be: " << stack_size << endl;
  }
  void prepareStack() {
    cout << "Sub sp register" << endl;
    string instr = toHexString(encodeAddSubImm(true, 31, 31, stack_size, false)).substr(2); // sub sp, sp, stack_size, substr to remove 0x prefix
    cout << format("Emit: sub sp, sp, {} | {}", stack_size, convertEndian(instr)) << endl;
    constructFullinstr(instr);
  }
  void printInitStack() {
    cout << "--- Printing initial stack ---" << endl;
    for (const auto &p : stackToVec) {
      cout << format("[sp, #{}] = {}[{}]", p.first, type_category_to_string(p.second.first), p.second.second) << endl;
    }
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
          [&offset, &general_reg_used, &fp_reg_used, &instr, &i, this](auto &&value) {
            char typeInfo = typeid(value).name()[0];
            vecToStack[{TypeCategory::PARAM, i}] = offset;
            stackToVec[offset] = {TypeCategory::PARAM, i};
            if (typeInfo == 'f') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_F32, fp_reg_used, 31, offset)).substr(2);
              loadType[{TypeCategory::PARAM, i}] = LdStType::LDR_F32;
              cout << format("Emit: str s{}, [sp, #{}] | {}", fp_reg_used, offset, convertEndian(instr)) << endl;
              offset -= 4;
              fp_reg_used += 1;
            } else if (typeInfo == 'd') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_F64, fp_reg_used, 31, offset)).substr(2);
              loadType[{TypeCategory::PARAM, i}] = LdStType::LDR_F64;
              cout << format("Emit: str d{}, [sp, #{}] | {}", fp_reg_used, offset, convertEndian(instr)) << endl;
              offset -= 8;
              fp_reg_used += 1;
            } else if (typeInfo == 'l') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_64, general_reg_used, 31, offset)).substr(2);
              loadType[{TypeCategory::PARAM, i}] = LdStType::LDR_64;
              cout << format("Emit: str x{}, [sp, #{}] | {}", general_reg_used, offset, convertEndian(instr)) << endl;
              offset -= 4;
              general_reg_used += 1;
            } else if (typeInfo == 'i') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_32, general_reg_used, 31, offset)).substr(2);
              loadType[{TypeCategory::PARAM, i}] = LdStType::LDR_32;
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
    int offset = local_stack_end_location;
    for (int i = 0; i < local_data.size(); ++i) {
      std::visit(
          [&offset, &instr, &i, this](auto &&value) {
            char typeInfo = typeid(value).name()[0];
            vecToStack[{TypeCategory::LOCAL, i}] = offset;
            stackToVec[offset] = {TypeCategory::LOCAL, i};
            if (typeInfo == 'f') {
              // NOTE: we are only moving zeros, so treat them like int here
              // xzr/wzr have same number as sp (31)
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_32, 31, 31, offset)).substr(2);
              loadType[{TypeCategory::LOCAL, i}] = LdStType::LDR_F32;
              cout << format("Emit: str wzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 4;
            } else if (typeInfo == 'd') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_64, 31, 31, offset)).substr(2);
              loadType[{TypeCategory::LOCAL, i}] = LdStType::LDR_F64;
              cout << format("Emit: str xzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 8;
            } else if (typeInfo == 'l') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_64, 31, 31, offset)).substr(2);
              loadType[{TypeCategory::LOCAL, i}] = LdStType::LDR_64;
              cout << format("Emit: str xzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 4;
            } else if (typeInfo == 'i') {
              instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_32, 31, 31, offset)).substr(2);
              loadType[{TypeCategory::LOCAL, i}] = LdStType::LDR_32;
              cout << format("Emit: str wzr, [sp, #{}] | {}", offset, convertEndian(instr)) << endl;
              offset -= 4;
            }
          },
          local_data[i]);
      constructFullinstr(instr);
    }
  }
  void restoreStack() {
    // getting result and restoring sp register
    cout << "Moving stack top to register as result" << endl;
    string prepare_ans_instr;
    int current_wasm_pointer = wasm_stack_pointer + 8;
    for (int i = 0; i < result_data.size(); ++i) {
      // we are iterating here
      // but we are actually expecting i=0 only (1 result)
      std::visit(
          [&i, &prepare_ans_instr, &current_wasm_pointer, this](auto &&value) {
            char typeInfo = typeid(value).name()[0];
            if (typeInfo == 'f') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'd') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'l') {
              prepare_ans_instr += toHexString(encodeLoadStoreUnsignedImm(LdStType::LDR_64, i, 31, current_wasm_pointer)).substr(2);
              cout << format("Emit: ldr x0, [sp, #{}] | {}", current_wasm_pointer, convertEndian(prepare_ans_instr)) << endl;
            } else if (typeInfo == 'i') {
              prepare_ans_instr += toHexString(encodeLoadStoreUnsignedImm(LdStType::LDR_32, i, 31, current_wasm_pointer)).substr(2);
              cout << format("Emit: ldr w0, [sp, #{}] | {}", current_wasm_pointer, convertEndian(prepare_ans_instr)) << endl;
            }
          },
          result_data[i]);
      current_wasm_pointer -= 8;
    }
    cout << "Restore sp register" << endl;
    const string restore_sp_instr =
        toHexString(encodeAddSubImm(false, 31, 31, stack_size, false)).substr(2); // add sp, sp, stack_size, substr to remove 0x prefix
    cout << format("Emit: add sp, sp, {} | {}", stack_size, convertEndian(restore_sp_instr)) << endl;
    constructFullinstr(prepare_ans_instr + restore_sp_instr);
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
    cout << "--- Loading params to their respective registers ---" << endl;
    if (param_data.size() == 0) {
      cout << "No params need to be load" << endl;
    }
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
              const string instr = WrapperEncodeMovInt64(i, value, RegType::X_REG);
              cout << format("Emit: mov x{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            } else if (typeInfo == 'i') {
              const string instr = WrapperEncodeMovInt32(i, value, RegType::W_REG);
              cout << format("Emit: mov s{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            }
          },
          param_data[i]);
    }
    cout << "---Loading parameters finished---" << endl;
  }
  int64_t executeInstr() {
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
      return -1;
    }
    // copy code to buffer
    memcpy(reinterpret_cast<void *>(instruction_set), charArray.get(), arraySize);
    // ensure memcpy isn't optimized away as a dead store.
    __builtin___clear_cache(reinterpret_cast<char *>(instruction_set), reinterpret_cast<char *>(instruction_set) + arraySize);
    // !不需要做任何传参，因为参数已经放在寄存器里啦
    int64_t ans = instruction_set();
    // WARN: reset instructions, very important if we want to call it again!
    pre_instructions_for_param_loading = "";
    instructions = "";
    if (result_data.size() == 0) {
      // in this case ans will still be x0
      // which is probably the first param of our function
      // setting it to zero as this value is meaningless
      ans = 0;
    }
    munmap(reinterpret_cast<void *>(instruction_set), arraySize);
    return ans;
  }
  void print_data(TypeCategory category) {
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
  void print_stack() {
    cout << "--- Printing stack---" << endl;
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
  void emitGet(const uint64_t var_to_get, TypeCategory vecType) {
    /**
     * Local.get i
     * push to wasm stack memory[var[i]]
     * var[i] -> x/w11 -> stack[top]
     */
    LdStType ldtype = loadType[{vecType, var_to_get}];
    int stack_offset = vecToStack[{vecType, var_to_get}];
    cout << format("Getting {}[{}]", type_category_to_string(vecType), var_to_get) << endl;
    // Note: We use x11 as a bridge register for memory -> memory transfer!
    string load_param_instr = toHexString(encodeLoadStoreUnsignedImm(ldtype, 11, 31, stack_offset)).substr(2);
    // var[i] -> x/w11
    string reg11 = (ldtype == LdStType::LDR_32) ? "w11" : ((ldtype == LdStType::LDR_64) ? "x11" : "Unknown type");
    cout << format("Emit: ldr {}, [sp, #{}] | {}", reg11, stack_offset, convertEndian(load_param_instr)) << endl;
    string store_to_stack_instr = toHexString(encodeLoadStoreUnsignedImm(convertLdSt(ldtype), 11, 31, wasm_stack_pointer)).substr(2);
    // x/w11 -> stack[top]
    cout << format("Emit: str {}, [sp, #{}] | {}", reg11, wasm_stack_pointer, convertEndian(store_to_stack_instr)) << endl;
    wasm_stack_pointer -= 8; // decrease wasm stack after push
    constructFullinstr(load_param_instr + store_to_stack_instr);
  }
  void emitSet(const uint64_t var_to_set, TypeCategory vecType, bool isTee = false) {
    /**
     * Local.set 0
     * Set memory[var[i]] to top value of wasm stack
     * stack[top] -> x/w11 -> var[i]
     */
    LdStType ldType = loadType[{vecType, var_to_set}];
    int stack_offset = vecToStack[{vecType, var_to_set}];
    cout << format("Assigning to {}[{}]", type_category_to_string(vecType), var_to_set) << endl;
    wasm_stack_pointer += 8;
    string store_to_stack_instr = toHexString(encodeLoadStoreUnsignedImm(ldType, 11, 31, wasm_stack_pointer)).substr(2);
    string reg11 = (ldType == LdStType::LDR_32) ? "w11" : ((ldType == LdStType::LDR_64) ? "x11" : "Unknown type");
    cout << format("Emit: ldr {}, [sp, #{}] | {}", reg11, wasm_stack_pointer, convertEndian(store_to_stack_instr)) << endl;
    string reg_to_mem_instr = toHexString(encodeLoadStoreUnsignedImm(convertLdSt(ldType), 11, 31, stack_offset)).substr(2);
    cout << format("Emit: str {}, [sp, #{}] | {}", reg11, stack_offset, convertEndian(reg_to_mem_instr)) << endl;
    if (isTee) {
      wasm_stack_pointer -= 8; // todo:!!
    }
    constructFullinstr(store_to_stack_instr + reg_to_mem_instr);
  }
  void emitConst(wasm_type elem) {
    /***
     * push value $elem onto wasm Stack
     * mov $elem, x11
     * str x11, [sp+wasm_stack_pointer]
     */
    std::visit(
        [this](auto &&value) {
          char typeInfo = typeid(value).name()[0];
          if (typeInfo == 'f') {
            // todo: don't support fmov yet
            throw std::invalid_argument("Don't support float const yet; need to properly implement fmov or store float first");
            // string load_to_reg_instr=toHexString(encodeFmovz()).substr(2);
            // string store_to_stack_instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_F32, 11, 31, wasm_stack_pointer, false)).substr(2);
          } else if (typeInfo == 'd') {
            throw std::invalid_argument("Don't support double const yet; need to properly implement fmov or store double first");
          } else if (typeInfo == 'i') {
            cout << format("i32.const {}", value) << endl;
            string load_to_reg_instr = WrapperEncodeMovInt32(11, value, RegType::W_REG);
            cout << format("Emit: mov {}, w11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
            string store_to_stack_instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_32, 11, 31, wasm_stack_pointer)).substr(2);
            cout << format("Emit: str w11, [sp, #{}] | {}", wasm_stack_pointer, convertEndian(store_to_stack_instr)) << endl;
            constructFullinstr(load_to_reg_instr + store_to_stack_instr);
          } else if (typeInfo == 'l') {
            cout << format("i64.const {}", value) << endl;
            string load_to_reg_instr = WrapperEncodeMovInt64(11, value, RegType::X_REG);
            cout << format("Emit: mov {}, x11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
            string store_to_stack_instr = toHexString(encodeLoadStoreUnsignedImm(LdStType::STR_64, 11, 31, wasm_stack_pointer)).substr(2);
            cout << format("Emit: str x11, [sp, #{}] | {}", wasm_stack_pointer, convertEndian(store_to_stack_instr)) << endl;
            constructFullinstr(load_to_reg_instr + store_to_stack_instr);
          }
        },
        elem);
    wasm_stack_pointer -= 8;
  }
  void emitArithOp(char typeInfo, char opType) {
    LdStType ldType;
    LdStType stType;
    RegType regtype;
    string opstr;
    switch (opType) {
    case '+':
      opstr = "add";
      break;
    case '-':
      opstr = "sub";
      break;
    case '*':
      opstr = "mul";
      break;
    case '/':
      opstr = "div";
      break;
    default:
      throw "Unknown arithmetic operator";
      break;
    }
    if (typeInfo == 'i') {
      ldType = LdStType::LDR_32;
      stType = LdStType::STR_32;
      regtype = W_REG;
      cout << format("i32.{}", opstr) << endl;
    } else if (typeInfo == 'l') {
      ldType = LdStType::LDR_64;
      stType = LdStType::STR_64;
      regtype = X_REG;
      cout << format("i64.{}", opstr) << endl;
    }
    wasm_stack_pointer += 8;
    // r11 = b
    string load_second_param_instr = toHexString(encodeLoadStoreUnsignedImm(ldType, 11, 31, wasm_stack_pointer)).substr(2);
    cout << format("Emit: ldr w11, [sp, #{}] | {}", wasm_stack_pointer, convertEndian(load_second_param_instr)) << endl;
    wasm_stack_pointer += 8;
    // r12 = a
    string load_first_param_instr = toHexString(encodeLoadStoreUnsignedImm(ldType, 12, 31, wasm_stack_pointer)).substr(2);
    cout << format("Emit: ldr w12, [sp, #{}] | {}", wasm_stack_pointer, convertEndian(load_first_param_instr)) << endl;
    // r11 = a op b
    cout << "Emit: ";
    string arith_instr;
    switch (opType) {
    case '+':
      arith_instr = toHexString(encodeAddSubShift(false, regtype, 11, 12, 11)).substr(2);
      break;
    case '-':
      arith_instr = toHexString(encodeAddSubShift(true, regtype, 11, 12, 11)).substr(2);
      break;
    case '*':
      arith_instr = toHexString(encodeMul(regtype, 11, 12, 11)).substr(2);
      break;
    case '/':
      break;
    default:
      throw "Unknown arithmetic operator";
      break;
    }
    string store_to_stack_instr = toHexString(encodeLoadStoreUnsignedImm(stType, 11, 31, wasm_stack_pointer)).substr(2);
    cout << format("Emit: str w11, [sp, #{}] | {}", wasm_stack_pointer, convertEndian(store_to_stack_instr)) << endl;
    wasm_stack_pointer -= 8; // decrease wasm stack after push
    constructFullinstr(load_first_param_instr + load_second_param_instr + arith_instr + store_to_stack_instr);
  }
  void runningWasmCode(int i) {
    cout << "--- JITing wasm code ---" << endl;
    wasm_stack_pointer = wasm_stack_end_location;
    cout << format("*Current wasm stack pointer is: {}", wasm_stack_pointer) << endl;

    while (i < code_vec.size()) {
      /**
       * WebAssembly Opcodes
       * https://pengowray.github.io/wasm-ops/
       */
      if (code_vec[i] == "0f") { // ret
        restoreStack();
        emitRet();
        ++i;
      } else if (code_vec[i] == "0b") { // end
        restoreStack();
        emitRet();
        ++i;
      } else if (code_vec[i] == "20") { // local.get
        u_int64_t var_to_get = stoul(code_vec[i + 1], nullptr, 16);
        cout << format("Local.get {}", var_to_get) << endl;
        if (var_to_get < param_data.size() + local_data.size()) {
          if (var_to_get < param_data.size()) {
            // falls within boundry of param variable
            emitGet(var_to_get, TypeCategory::PARAM);
            stack.push_back(param_data[var_to_get]);
          } else {
            // must be local variable then.
            var_to_get -= param_data.size();
            emitGet(var_to_get, TypeCategory::LOCAL);
            stack.push_back(local_data[var_to_get]);
          }
          i += 2;
        } else {
          cout << "Too big index {" + to_string(var_to_get) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
      } else if (code_vec[i] == "21") { // local.set
        u_int64_t var_to_set = stoul(code_vec[i + 1], nullptr, 16);
        cout << format("Local.set {}", var_to_set) << endl;
        if (var_to_set < param_data.size() + local_data.size()) {
          if (var_to_set < param_data.size()) {
            emitSet(var_to_set, TypeCategory::PARAM);
            param_data[var_to_set] = stack.back();
          } else {
            var_to_set -= param_data.size();
            emitSet(var_to_set, TypeCategory::LOCAL);
            local_data[var_to_set] = stack.back();
          }
          stack.pop_back();
          i += 2;
        } else {
          cout << "Too big index {" + to_string(var_to_set) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
      } else if (code_vec[i] == "22") { // local.tee
        u_int64_t var_to_tee = stoul(code_vec[i + 1], nullptr, 16);
        cout << format("Local.tee {}", var_to_tee) << endl;
        if (var_to_tee < param_data.size() + local_data.size()) {
          if (var_to_tee < param_data.size()) {
            emitSet(var_to_tee, TypeCategory::PARAM, true);
            param_data[var_to_tee] = stack.back();
          } else {
            var_to_tee -= param_data.size();
            emitSet(var_to_tee, TypeCategory::LOCAL, true);
            local_data[var_to_tee] = stack.back();
          }
          i += 2;
        } else {
          cout << "Too big index {" + to_string(var_to_tee) + "} for local data; skipping current op;" << endl;
          i += 2;
          continue;
        }
      } else if (code_vec[i] == "41") { // i32.const
        wasm_type elem = static_cast<int32_t>(stoul(code_vec[i + 1], nullptr, 16));
        emitConst(elem);
        stack.push_back(elem);
        i += 2;
      } else if (code_vec[i] == "42") { // i64.const
        wasm_type elem = static_cast<int64_t>(stoul(code_vec[i + 1], nullptr, 16));
        emitConst(elem);
        stack.push_back(elem);
        i += 2;
      } else if (code_vec[i] == "43") { // f32.const
        wasm_type elem = hexToFloat(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4]);
        emitConst(elem);
        stack.push_back(elem);
        i += 5;
      } else if (code_vec[i] == "44") { // f64.const
        wasm_type elem =
            hexToDouble(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4] + code_vec[i + 5] + code_vec[i + 6] + code_vec[i + 7]);
        emitConst(elem);
        stack.push_back(elem);
        i += 9;
      } else if (code_vec[i] == "6a") { // i32.add
        emitArithOp('i', '+');
        auto b = stack.back();
        stack.pop_back();
        auto a = stack.back();
        stack.pop_back();
        stack.push_back(a + b);
        i += 1;
      } else if (code_vec[i] == "6b") { // i32.sub
        emitArithOp('i', '-');
        auto b = stack.back();
        stack.pop_back();
        auto a = stack.back();
        stack.pop_back();
        stack.push_back(a - b);
        i += 1;
      } else if (code_vec[i] == "6c") { // i32.mul
        emitArithOp('i', '*');
        auto b = stack.back();
        stack.pop_back();
        auto a = stack.back();
        stack.pop_back();
        stack.push_back(a * b);
        i += 1;
      } else if (code_vec[i] == "7c") { // i64.add
        emitArithOp('l', '+');
        auto b = stack.back();
        stack.pop_back();
        auto a = stack.back();
        stack.pop_back();
        stack.push_back(a + b);
        i += 1;
      } else if (code_vec[i] == "7d") { // i64.sub
        emitArithOp('l', '-');
        auto b = stack.back();
        stack.pop_back();
        auto a = stack.back();
        stack.pop_back();
        stack.push_back(a - b);
        i += 1;
      } else if (code_vec[i] == "7e") { // i64.mul
        emitArithOp('l', '*');
        auto b = stack.back();
        stack.pop_back();
        auto a = stack.back();
        stack.pop_back();
        stack.push_back(a * b);
        i += 1;
      }
      cout << format("*Current wasm stack pointer is: {}", wasm_stack_pointer) << endl;
    }
  }

  vector<string> code_vec;
  u_int64_t local_var_declare_count = 0;
  string instructions;
  string pre_instructions_for_param_loading;
  vector<wasm_type> local_data;
  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
  vector<wasm_type> stack;
  map<pair<TypeCategory, int>, int> vecToStack;    // {TypeCategory::PARAM, 0} : 0x4
  map<pair<TypeCategory, int>, LdStType> loadType; // {TypeCategory::PARAM, 0}: LDR32
  map<int, pair<TypeCategory, int>> stackToVec;    // 0x4 : {TypeCategory::PARAM: 0}
  int stack_size = 0;
  int param_stack_start_location = 0;
  int param_stack_end_location = 0;
  int local_stack_start_location = 0;
  int local_stack_end_location = 0;
  int wasm_stack_start_location = 0;
  int wasm_stack_end_location = 0;
  int wasm_stack_pointer = 0;
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