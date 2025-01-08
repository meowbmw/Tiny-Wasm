#pragma once
#include "FloatUtils.h"
#include "Opcode.hpp"
#include "OverloadOperator.h"
#include "Utils.h"
using namespace std;
using ArithOperation = std::function<wasm_type(wasm_type, wasm_type)>;

class WasmFunction {
public:
  void processCodeVec() {
    int offset = 0;
    for (int i = 0; i < local_var_declare_count; ++i) {
      const unsigned int var_count_in_this_declare = stoul(code_vec[offset], nullptr, 16);
      const string var_type_in_this_declare = code_vec[offset + 1];
      for (int j = 0; j < var_count_in_this_declare; ++j) {
        add_data(TypeCategory::LOCAL, var_type_in_this_declare);
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
    string instr = encodeAddSubImm(X_REG, true, 31, 31, stack_size); // sub sp, sp, stack_size
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
              instr = encodeLoadStoreUnsignedImm(S_REG, STR, fp_reg_used, 31, offset);
              regTypeGetter[{TypeCategory::PARAM, i}] = S_REG;
              offset -= 4;
              fp_reg_used += 1;
            } else if (typeInfo == 'd') {
              instr = encodeLoadStoreUnsignedImm(D_REG, STR, fp_reg_used, 31, offset);
              regTypeGetter[{TypeCategory::PARAM, i}] = D_REG;
              offset -= 8;
              fp_reg_used += 1;
            } else if (typeInfo == 'l') {
              instr = encodeLoadStoreUnsignedImm(X_REG, STR, general_reg_used, 31, offset);
              regTypeGetter[{TypeCategory::PARAM, i}] = X_REG;
              offset -= 4;
              general_reg_used += 1;
            } else if (typeInfo == 'i') {
              instr = encodeLoadStoreUnsignedImm(W_REG, STR, general_reg_used, 31, offset);
              regTypeGetter[{TypeCategory::PARAM, i}] = W_REG;
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
              instr = encodeLoadStoreUnsignedImm(W_REG, STR, 31, 31, offset);
              regTypeGetter[{TypeCategory::LOCAL, i}] = S_REG;
              offset -= 4;
            } else if (typeInfo == 'd') {
              instr = encodeLoadStoreUnsignedImm(X_REG, STR, 31, 31, offset);
              regTypeGetter[{TypeCategory::LOCAL, i}] = D_REG;
              offset -= 8;
            } else if (typeInfo == 'l') {
              instr = encodeLoadStoreUnsignedImm(X_REG, STR, 31, 31, offset);
              regTypeGetter[{TypeCategory::LOCAL, i}] = X_REG;
              offset -= 4;
            } else if (typeInfo == 'i') {
              instr = encodeLoadStoreUnsignedImm(W_REG, STR, 31, 31, offset);
              regTypeGetter[{TypeCategory::LOCAL, i}] = W_REG;
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
              prepare_ans_instr += encodeLoadStoreUnsignedImm(X_REG, LDR, i, 31, current_wasm_pointer);
            } else if (typeInfo == 'i') {
              prepare_ans_instr += encodeLoadStoreUnsignedImm(W_REG, LDR, i, 31, current_wasm_pointer);
            }
          },
          result_data[i]);
      current_wasm_pointer -= 8;
    }
    cout << "Restore sp register" << endl;
    const string restore_sp_instr = encodeAddSubImm(X_REG, false, 31, 31, stack_size); // add sp, sp, stack_size
    constructFullinstr(prepare_ans_instr + restore_sp_instr);
  }
  void emitRet() {
    const string instr = "C0035FD6";
    cout << format("Emit: ret | {}", instr) << endl;
    constructFullinstr(instr);
  }
  void constructFullinstr(string sub_instr) {
    wasm_instructions = wasm_instructions + sub_instr;
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
    int backReg = 15;
    cout << "Backing up x0 buffer to x" << backReg << endl;
    pre_instructions_for_param_loading += encodeMovRegister(X_REG, backReg, 0);
    cout << "Loading parameters" << endl;
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
              // cout << format("Emit: mov x{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            } else if (typeInfo == 'i') {
              const string instr = WrapperEncodeMovInt32(i, value, RegType::W_REG);
              // cout << format("Emit: mov s{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            }
          },
          param_data[i]);
    }
    cout << "---Loading parameters finished---" << endl;
  }
  void resetAfterExecution() {
    pre_instructions_for_param_loading = "";
    wasm_instructions = ""; // no need to reset this?
    stack.clear();
  }
  static int64_t executeInstr(const string &full_instructions, const string &pre_instructions_for_param_loading = "",
                              const string &wasm_instructions = "") {
    /**
     * Making this function static so it can be called freely
     *
     * Allocate memory with execute permission
     * And load machine code into that
     *
     */
    cout << "Machine instruction to load: " << full_instructions << endl;
    if (pre_instructions_for_param_loading.size() > 0) {
      cout << " - Load param instr: ";
      for (size_t i = 0; i < pre_instructions_for_param_loading.size(); i += 8) {
        if (i > 0) {
          std::cout << " | ";
        }
        std::cout << pre_instructions_for_param_loading.substr(i, 8);
      }
      cout << endl;
    }
    if (wasm_instructions.size() > 0) {
      cout << " - Run wasm instr: ";
      for (size_t i = 0; i < wasm_instructions.size(); i += 8) {
        if (i > 0) {
          std::cout << " | ";
        }
        std::cout << wasm_instructions.substr(i, 8);
      }
      cout << endl;
    }
    const size_t arraySize = full_instructions.length() / 2;
    auto charArray = make_unique<unsigned char[]>(arraySize); // use smart pointer here so we don't need to free it manually
    for (size_t i = 0; i < arraySize; i++) {
      const string byteStr = full_instructions.substr(i * 2, 2);
      charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    int64_t (*instruction_set)(void *) = nullptr;
    void *buffer = malloc(1024);
    // allocate executable buffer
    instruction_set =
        reinterpret_cast<int64_t (*)(void *)>(mmap(nullptr, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (instruction_set == MAP_FAILED) {
      perror("mmap");
      return -1;
    }
    // copy code to buffer
    memcpy(reinterpret_cast<void *>(instruction_set), charArray.get(), arraySize);
    // ensure memcpy isn't optimized away as a dead store.
    __builtin___clear_cache(reinterpret_cast<char *>(instruction_set), reinterpret_cast<char *>(instruction_set) + arraySize);
    // !不需要做任何传参，因为参数已经放在寄存器里啦
    int64_t ans = instruction_set(buffer);
    munmap(reinterpret_cast<void *>(instruction_set), arraySize);
    return ans;
  }
  int64_t executeInstr() {
    // Wrapper function because we want to support calling executeInstr(instr) without class instantiate
    // Warn: Append pre wasm_instructions here
    string full_instructions = pre_instructions_for_param_loading + encodeBranch(1) + wasm_instructions;
    int64_t ans = executeInstr(full_instructions, pre_instructions_for_param_loading, wasm_instructions);
    // WARN: reset, very important if we want to call it again!
    resetAfterExecution();
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
    cout << "--- Printing stack (This is not wasm stack!) ---" << endl;
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
  void set_code_vec(vector<string> &v, size_t l = 0) {
    code_vec = v;
    local_var_declare_count = l;
  }
  void emitSaveBeforeFunctionCall() {
    /** Store caller saved registers before function calls
     *  sub sp, sp, #192
        stp x0, x1, [sp, #0]
        stp x2, x3, [sp, #16]
        stp x4, x5, [sp, #32]
        stp x6, x7, [sp, #48]
        stp x8, x9, [sp, #64]
        stp x10, x11, [sp, #80]
        stp x12, x13, [sp, #96]
        stp x14, x15, [sp, #112]
        stp x16, x17, [sp, #128]
        stp x18, x29, [sp, #144]
        stp x30, xzr, [sp, #160]
     */
    cout << "*Emitting Instruction to save registers before function call:" << endl;
    string instr;
    LdStType ldstType = LdStType::STR;
    instr += encodeAddSubImm(X_REG, true, 31, 31, 192);
    instr += encodeLdpStp(X_REG, ldstType, 0, 1, 31, 0);
    instr += encodeLdpStp(X_REG, ldstType, 2, 3, 31, 16);
    instr += encodeLdpStp(X_REG, ldstType, 4, 5, 31, 32);
    instr += encodeLdpStp(X_REG, ldstType, 6, 7, 31, 48);
    instr += encodeLdpStp(X_REG, ldstType, 8, 9, 31, 64);
    instr += encodeLdpStp(X_REG, ldstType, 10, 11, 31, 80);
    instr += encodeLdpStp(X_REG, ldstType, 12, 13, 31, 96);
    instr += encodeLdpStp(X_REG, ldstType, 14, 15, 31, 112);
    instr += encodeLdpStp(X_REG, ldstType, 16, 17, 31, 128);
    instr += encodeLdpStp(X_REG, ldstType, 18, 29, 31, 144);
    instr += encodeLdpStp(X_REG, ldstType, 30, 31, 31, 160);
    wasm_instructions += instr;
  }
  void emitRestoreAfterFunctionCall() {
    /** Restore caller saved registers after return from function calls
     *  ldp x0, x1, [sp, #0]
        ldp x2, x3, [sp, #16]
        ldp x4, x5, [sp, #32]
        ldp x6, x7, [sp, #48]
        ldp x8, x9, [sp, #64]
        ldp x10, x11, [sp, #80]
        ldp x12, x13, [sp, #96]
        ldp x14, x15, [sp, #112]
        ldp x16, x17, [sp, #128]
        ldp x18, x29, [sp, #144]
        ldp x30, xzr, [sp, #160]
        add sp, sp, #192
     */
    cout << "*Emitting Instruction to restore registers after function call:" << endl;
    string instr;
    LdStType ldstType = LdStType::LDR;
    instr += encodeLdpStp(X_REG, ldstType, 0, 1, 31, 0);
    instr += encodeLdpStp(X_REG, ldstType, 2, 3, 31, 16);
    instr += encodeLdpStp(X_REG, ldstType, 4, 5, 31, 32);
    instr += encodeLdpStp(X_REG, ldstType, 6, 7, 31, 48);
    instr += encodeLdpStp(X_REG, ldstType, 8, 9, 31, 64);
    instr += encodeLdpStp(X_REG, ldstType, 10, 11, 31, 80);
    instr += encodeLdpStp(X_REG, ldstType, 12, 13, 31, 96);
    instr += encodeLdpStp(X_REG, ldstType, 14, 15, 31, 112);
    instr += encodeLdpStp(X_REG, ldstType, 16, 17, 31, 128);
    instr += encodeLdpStp(X_REG, ldstType, 18, 29, 31, 144);
    instr += encodeLdpStp(X_REG, ldstType, 30, 31, 31, 160);
    instr += encodeAddSubImm(X_REG, false, 31, 31, 192);
    wasm_instructions += instr;
  }
  void emitSetJmp() {
    /**
     *  	stp	x19, x20, [x0, 0<<3]
          stp	x21, x22, [x0, 2<<3]
          stp	x23, x24, [x0, 4<<3]
          stp	x25, x26, [x0, 6<<3]
          stp	x27, x28, [x0, 8<<3]
          stp	x29, x30, [x0, 10<<3]
          mov	x2,  sp
          str	x2,  [x0, 13<<3]
          mov	w0, #0
          ret
     */
    string instr;
  }
  void emitLongJmp() {
    /**
     *  ldp	x19, x20, [x0, 0<<3]
        ldp	x21, x22, [x0, 2<<3]
        ldp	x23, x24, [x0, 4<<3]
        ldp	x25, x26, [x0, 6<<3]
        ldp	x27, x28, [x0, 8<<3]
              ldp	x29, x30, [x0, 10<<3]
        ldr	x5, [x0, 13<<3]; x5 <- [x0, 104]
        mov	sp, x5; sp <- x5
        cmp	x1, #0
        mov	x0, #1
        csel	x0, x1, x0, ne
        br	x30
     */
  }
  void emitGet(const uint64_t var_to_get, TypeCategory vecType) {
    /**
     * Local.get i
     * push to wasm stack memory[var[i]]
     * var[i] -> x/w11 -> stack[top]
     */
    RegType regtype = regTypeGetter[{vecType, var_to_get}];
    int stack_offset = vecToStack[{vecType, var_to_get}];
    cout << format("Getting {}[{}]", type_category_to_string(vecType), var_to_get) << endl;
    // Note: We use x11 as a bridge register for memory -> memory transfer!
    string load_param_instr = encodeLoadStoreUnsignedImm(regtype, LDR, 11, 31, stack_offset);
    // var[i] -> x/w11
    string store_to_stack_instr = encodeLoadStoreUnsignedImm(regtype, STR, 11, 31, wasm_stack_pointer);
    // x/w11 -> stack[top]
    wasm_stack_pointer -= 8; // decrease wasm stack after push
    constructFullinstr(load_param_instr + store_to_stack_instr);
  }
  void emitSet(const uint64_t var_to_set, TypeCategory vecType, bool isTee = false) {
    /**
     * Local.set 0
     * Set memory[var[i]] to top value of wasm stack
     * stack[top] -> x/w11 -> var[i]
     */
    RegType regtype = regTypeGetter[{vecType, var_to_set}];
    int stack_offset = vecToStack[{vecType, var_to_set}];
    cout << format("Assigning to {}[{}]", type_category_to_string(vecType), var_to_set) << endl;
    wasm_stack_pointer += 8;
    string store_to_stack_instr = encodeLoadStoreUnsignedImm(regtype, LDR, 11, 31, wasm_stack_pointer);
    string reg_to_mem_instr = encodeLoadStoreUnsignedImm(regtype, STR, 11, 31, stack_offset);
    if (isTee) {
      wasm_stack_pointer -= 8; // teeing keeps stack intact
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
            // cout << format("Emit: mov {}, w11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
            string store_to_stack_instr = encodeLoadStoreUnsignedImm(W_REG, STR, 11, 31, wasm_stack_pointer);
            constructFullinstr(load_to_reg_instr + store_to_stack_instr);
          } else if (typeInfo == 'l') {
            cout << format("i64.const {}", value) << endl;
            string load_to_reg_instr = WrapperEncodeMovInt64(11, value, RegType::X_REG);
            // cout << format("Emit: mov {}, x11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
            string store_to_stack_instr = encodeLoadStoreUnsignedImm(X_REG, STR, 11, 31, wasm_stack_pointer);
            constructFullinstr(load_to_reg_instr + store_to_stack_instr);
          }
        },
        elem);
    wasm_stack_pointer -= 8;
  }
  void emitArithOp(char typeInfo, char opType, bool isSigned = true) {
    /*
     * A wrapper for common arithmatic operations: +, -, *, /
     */
    // Note: isSigned is only used to differentiate div_s and div_u
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
      if (isSigned) {
        opstr += "_s";
      } else {
        opstr += "_u";
      }
      break;
    default:
      throw "Unknown arithmetic operator";
      break;
    }
    if (typeInfo == 'i') {
      regtype = W_REG;
      cout << format("i32.{}", opstr) << endl;
    } else if (typeInfo == 'l') {
      regtype = X_REG;
      cout << format("i64.{}", opstr) << endl;
    }
    wasm_stack_pointer += 8;
    // r11 = b
    string load_second_param_instr = encodeLoadStoreUnsignedImm(regtype, LDR, 11, 31, wasm_stack_pointer);
    wasm_stack_pointer += 8;
    // r12 = a
    string load_first_param_instr = encodeLoadStoreUnsignedImm(regtype, LDR, 12, 31, wasm_stack_pointer);
    // r11 = a op b
    string arith_instr;
    string check_div_instr;
    string branch_equal_zero_instr;
    switch (opType) {
    case '+':
      arith_instr = encodeAddSubShift(false, regtype, 11, 12, 11);
      break;
    case '-':
      arith_instr = encodeAddSubShift(true, regtype, 11, 12, 11);
      break;
    case '*':
      arith_instr = encodeMul(regtype, 11, 12, 11);
      break;
    case '/':
      check_div_instr = encodeCompareShift(regtype, 12, 11);
      branch_equal_zero_instr = encodeBranchCondition(1, reverse_cond_str_map["eq"]);
      arith_instr = encodeDiv(regtype, isSigned, 11, 12, 11);
      break;
    default:
      throw "Unknown arithmetic operator";
      break;
    }
    string store_to_stack_instr = encodeLoadStoreUnsignedImm(regtype, STR, 11, 31, wasm_stack_pointer);
    wasm_stack_pointer -= 8; // decrease wasm stack after push
    constructFullinstr(load_first_param_instr + load_second_param_instr + arith_instr + store_to_stack_instr);
  }
  void commonStackOp(char opType) {
    auto b = stack.back();
    stack.pop_back();
    auto a = stack.back();
    stack.pop_back();
    stack.push_back(operations_map[opType](a, b));
  }
  void commonLocalOp(int i, string opType) {
    u_int64_t var_index = stoul(code_vec[i + 1], nullptr, 16);
    cout << format("Local.{} {}", opType, var_index) << endl;
    TypeCategory typecategory;
    if (var_index < param_data.size() + local_data.size()) {
      if (var_index < param_data.size()) {
        // falls within boundry of param variable
        typecategory = TypeCategory::PARAM;
      } else {
        // must be local variable then.
        typecategory = TypeCategory::LOCAL;
        var_index -= param_data.size();
      }
      if (opType == "get") {
        emitGet(var_index, typecategory);
        if (typecategory == TypeCategory::PARAM) {
          stack.push_back(param_data[var_index]);
        } else {
          stack.push_back(local_data[var_index]);
        }
      } else if (opType == "set") {
        emitSet(var_index, typecategory);
        if (typecategory == TypeCategory::PARAM) {
          param_data[var_index] = stack.back();
        } else {
          local_data[var_index] = stack.back();
        }
        stack.pop_back();
      } else if (opType == "tee") {
        emitSet(var_index, typecategory, true);
        if (typecategory == TypeCategory::PARAM) {
          param_data[var_index] = stack.back();
        } else {
          local_data[var_index] = stack.back();
        }
      } else {
        cout << "Unknown Local operation" << endl;
        throw "Unknown Local operation";
      }
    } else {
      throw "Too big index {" + to_string(var_index) + "} for local data; skipping current op;";
    }
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
        commonLocalOp(i, "get");
        i += 2;
      } else if (code_vec[i] == "21") { // local.set
        commonLocalOp(i, "set");
        i += 2;
      } else if (code_vec[i] == "22") { // local.tee
        commonLocalOp(i, "tee");
        i += 2;
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
        commonStackOp('+');
        i += 1;
      } else if (code_vec[i] == "6b") { // i32.sub
        emitArithOp('i', '-');
        commonStackOp('-');
        i += 1;
      } else if (code_vec[i] == "6c") { // i32.mul
        emitArithOp('i', '*');
        commonStackOp('*');
        i += 1;
      } else if (code_vec[i] == "6d") { // i32.div_s
        emitArithOp('i', '/', true);
        commonStackOp('/');
        i += 1;
      } else if (code_vec[i] == "6e") { // i32.div_u
        emitArithOp('i', '/', false);
        commonStackOp('/');
        i += 1;
      } else if (code_vec[i] == "7c") { // i64.add
        emitArithOp('l', '+');
        commonStackOp('+');
        i += 1;
      } else if (code_vec[i] == "7d") { // i64.sub
        emitArithOp('l', '-');
        commonStackOp('-');
        i += 1;
      } else if (code_vec[i] == "7e") { // i64.mul
        emitArithOp('l', '*');
        commonStackOp('*');
        i += 1;
      } else if (code_vec[i] == "7f") { // i64.div_s
        emitArithOp('l', '/', true);
        commonStackOp('/');
        i += 1;
      } else if (code_vec[i] == "80") { // i64.div_u
        emitArithOp('l', '/', false);
        commonStackOp('/');
        i += 1;
      }
      cout << format("*Current wasm stack pointer is: {}", wasm_stack_pointer) << endl;
    }
  }
  WasmFunction() {
    // initiate arithmetic operations map
    operations_map['+'] = [](wasm_type a, wasm_type b) {
      return a + b;
    };
    operations_map['-'] = [](wasm_type a, wasm_type b) {
      return a - b;
    };
    operations_map['*'] = [](wasm_type a, wasm_type b) {
      return a * b;
    };
    operations_map['/'] = [](wasm_type a, wasm_type b) {
      std::visit(
          [](auto &&value) {
            if (value == 0)
              cout << "! Division by zero in wasm code" << endl;
            return wasm_type(0);
            // disabled for now to check arm64 trap!!
            // throw std::runtime_error("Division by zero");
          },
          b);
      return a / b;
    };
  }
  vector<string> code_vec;
  u_int64_t local_var_declare_count = 0;
  string wasm_instructions;
  map<char, ArithOperation> operations_map;
  string pre_instructions_for_param_loading;
  vector<wasm_type> local_data;
  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
  vector<wasm_type> stack;
  map<pair<TypeCategory, int>, int> vecToStack;        // {TypeCategory::PARAM, 0} : 0x4
  map<pair<TypeCategory, int>, RegType> regTypeGetter; // {TypeCategory::PARAM, 0}: LDR32
  map<int, pair<TypeCategory, int>> stackToVec;        // 0x4 : {TypeCategory::PARAM: 0}
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