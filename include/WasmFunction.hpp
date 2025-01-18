#pragma once
#include "FloatUtils.h"
#include "Opcode.hpp"
#include "OverloadOperator.h"
#include "Utils.h"
using namespace std;
using ArithOperation = std::function<wasm_type(wasm_type, wasm_type)>;

uint8_t BACK_REG = 13;
string getSetJmpInstr() {
  /**
   *  	stp	x19, x20, [x0, 0]
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
  LdStType ldstType = LdStType::STR;
  instr += encodeLdpStp(X_REG, ldstType, 19, 20, 0, 0);
  instr += encodeLdpStp(X_REG, ldstType, 21, 22, 0, 2 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 23, 24, 0, 4 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 25, 26, 0, 6 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 27, 28, 0, 8 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 29, 30, 0, 10 << 3);
  instr += encodeMovSP(X_REG, 2, 31);
  instr += encodeLoadStoreImm(X_REG, ldstType, 2, 0, 13 << 3);
  instr += encodeMovz(0, 0, W_REG, 0);
  instr += encodeReturn();
  // cout << "Function SetJmp: " << instr << endl;
  return instr;
}
string getLongJmpInstr() {
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
  string instr;
  LdStType ldstType = LdStType::LDR;
  instr += encodeLdpStp(X_REG, ldstType, 19, 20, 0, 0 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 21, 22, 0, 2 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 23, 24, 0, 4 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 25, 26, 0, 6 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 27, 28, 0, 8 << 3);
  instr += encodeLdpStp(X_REG, ldstType, 29, 30, 0, 10 << 3);
  instr += encodeLoadStoreImm(X_REG, ldstType, 5, 0, 13 << 3);
  instr += encodeMovSP(X_REG, 31, 5);
  instr += encodeCompareImm(X_REG, 1, 0);
  instr += encodeMovz(0, 1, X_REG, 0);
  instr += encodeCSEL(X_REG, 0, 1, 0, reverse_cond_str_map.at("ne"));
  instr += encodeBranchRegister(30);
  // cout << "Function LongJmp: " << instr << endl;
  return instr;
}

class WasmFunction {
public:
  void processCodeVec() {
    int offset = 0;
    local_var_initialize(offset); // doesn't modify wasm_instructions
    printOriginWasmOpcode(offset);

    // start processing wasm_instructions here
    fakeInsertBranch("entry", BranchType()); // b main

    insertLabel("setjmp");
    wasm_instructions += getSetJmpInstr();

    insertLabel("preparelongjmp");
    wasm_instructions += encodeMovRegister(X_REG, 14, 30); // x14 <- x30
    wasm_instructions += encodeMovSP(X_REG, 15, 31);       // x15 <- sp
    wasm_instructions += encodeMovRegister(X_REG, 0, 13);  // x0 <- x13
    fakeInsertBranch("longjmp", BranchType());

    insertLabel("longjmp");
    wasm_instructions += getLongJmpInstr();

    insertLabel("raiseException");
    // doing this because the sp we backed up is wrong, its after stp (sp is decreased)
    // also x30 is set to the instruction after bl setjmp, not the origin return address, so it's also wrong
    wasm_instructions += encodeMovRegister(X_REG, 30, 14); // x30 <- x14
    wasm_instructions += encodeMovSP(X_REG, 31, 15);       // sp <- x31

    // store return code 1 to [x13]
    wasm_instructions += encodeMovz(11, 0x1, X_REG, 0);                   // x11=1, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, BACK_REG, 0); // [x13]=x11=1
    fakeInsertBranch("finalize", BranchType());

    insertLabel("entry");
    main_entry_initialize(offset);

    wrapper_setjmp();
    jiting_wasm_code(offset);

    // store return code 0 to x[0]
    wasm_instructions += encodeMovz(11, 0x0, X_REG, 0);                   // x11=0, this register can be any that is not used and caller-saved
    wasm_instructions += encodeLoadStoreImm(X_REG, STR, 11, BACK_REG, 0); // [x13]=x11=0

    insertLabel("finalize");
    main_entry_finalize();
    streambuf *old = cout.rdbuf();
    cout.rdbuf(0);
    fixUpfakeBranch();
    cout.rdbuf(old);
  }
  void printOriginWasmOpcode(int &offset) {
    cout << "Origin WASM Opcode: ";
    for (int i = offset; i < code_vec.size(); ++i) {
      cout << code_vec[i] << " ";
    }
    cout << endl;
  }
  void local_var_initialize(int &offset) {
    for (int i = 0; i < local_var_declare_count; ++i) {
      const unsigned int var_count_in_this_declare = stoul(code_vec[offset], nullptr, 16);
      const string var_type_in_this_declare = code_vec[offset + 1];
      for (int j = 0; j < var_count_in_this_declare; ++j) {
        add_data(TypeCategory::LOCAL, var_type_in_this_declare);
      }
      offset += 2;
    }
    // print_data(TypeCategory::PARAM);
    // print_data(TypeCategory::LOCAL);
  }
  void main_entry_initialize(int &offset) {
    getStackPreallocateSize(offset);
    prepareStack();
    initParam(); // initParam is storing to memory, prepareParams is storing to registers
    initLocal();
    // printInitStack();
  }
  void main_entry_finalize() {
    restoreStack();
    emitRet();
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
    streambuf *old = cout.rdbuf();
    cout.rdbuf(0);
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
    cout.rdbuf(old);
  }
  void prepareStack() {
    cout << "Sub sp register" << endl;
    string instr = encodeAddSubImm(X_REG, true, 31, 31, stack_size); // sub sp, sp, stack_size
    constructFullinstr(instr);
  }
  void printInitStack() {
    cout << "--- Printing initial stack ---" << endl;
    for (const auto &p : stackToVec) {
      cout << format("[sp, #0x{:x}] = {}[{}]", p.first, type_category_to_string(p.second.first), p.second.second) << endl;
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
              instr = encodeLoadStoreImm(S_REG, STR, fp_reg_used, 31, offset);
              regTypeGetter[{TypeCategory::PARAM, i}] = S_REG;
              offset -= 4;
              fp_reg_used += 1;
            } else if (typeInfo == 'd') {
              instr = encodeLoadStoreImm(D_REG, STR, fp_reg_used, 31, offset);
              regTypeGetter[{TypeCategory::PARAM, i}] = D_REG;
              offset -= 8;
              fp_reg_used += 1;
            } else if (typeInfo == 'l') {
              instr = encodeLoadStoreImm(X_REG, STR, general_reg_used, 31, offset);
              regTypeGetter[{TypeCategory::PARAM, i}] = X_REG;
              offset -= 4;
              general_reg_used += 1;
            } else if (typeInfo == 'i') {
              instr = encodeLoadStoreImm(W_REG, STR, general_reg_used, 31, offset);
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
              instr = encodeLoadStoreImm(W_REG, STR, 31, 31, offset);
              regTypeGetter[{TypeCategory::LOCAL, i}] = S_REG;
              offset -= 4;
            } else if (typeInfo == 'd') {
              instr = encodeLoadStoreImm(X_REG, STR, 31, 31, offset);
              regTypeGetter[{TypeCategory::LOCAL, i}] = D_REG;
              offset -= 8;
            } else if (typeInfo == 'l') {
              instr = encodeLoadStoreImm(X_REG, STR, 31, 31, offset);
              regTypeGetter[{TypeCategory::LOCAL, i}] = X_REG;
              offset -= 4;
            } else if (typeInfo == 'i') {
              instr = encodeLoadStoreImm(W_REG, STR, 31, 31, offset);
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
      // todo: we should be iterating here; i < result.size()
      // but we are actually expecting i=0 only (1 result)
      std::visit(
          [&i, &prepare_ans_instr, &current_wasm_pointer, this](auto &&value) {
            char typeInfo = typeid(value).name()[0];
            if (typeInfo == 'f') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'd') {
              throw std::invalid_argument("Fmov not supported yet!");
            } else if (typeInfo == 'l') {
              prepare_ans_instr += encodeLoadStoreImm(X_REG, LDR, i, 31, current_wasm_pointer);
            } else if (typeInfo == 'i') {
              prepare_ans_instr += encodeLoadStoreImm(W_REG, LDR, i, 31, current_wasm_pointer);
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
    string instr = encodeReturn();
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
    int backReg = BACK_REG;
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
              const string instr = WrapperEncodeMovInt64(i, value);
              // cout << format("Emit: mov x{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            } else if (typeInfo == 'i') {
              const string instr = WrapperEncodeMovInt32(i, value);
              // cout << format("Emit: mov s{}, {} | {}", i, value, convertEndian(instr)) << endl;
              pre_instructions_for_param_loading += instr;
            }
          },
          param_data[i]);
    }
    cout << "---Loading parameters finished---" << endl;
  }
  void resetAfterExecution() {
    pre_instructions_for_param_loading.clear();
    wasm_instructions.clear(); // no need to reset this?
    stack.clear();
    fake_insert_map.clear();
    label_map.clear();
  }
  void wrapper_setjmp() {
    wasm_instructions += encodeLdpStp(X_REG, STR, 29, 30, 31, -0x20, EncodingMode::PreIndex); // stp x29, x30, [sp, #-0x20]!

    wasm_instructions += encodeMovRegister(X_REG, 0, BACK_REG); // x0 <- x13
    fakeInsertBranch("setjmp", BranchType(false, true));        // bl setjmp

    wasm_instructions += encodeCompareImm(X_REG, 0, 0);
    fakeInsertBranch("raiseException", BranchType(true, false, reverse_cond_str_map.at("ne"))); // todo: if not equal, goto exception handling
    wasm_instructions += encodeLdpStp(X_REG, LDR, 29, 30, 31, 0x20, EncodingMode::PostIndex);   // ldp x29, x30, [sp], #0x20
  }
  void fakeInsertBranch(string label, BranchType branchType) {
    /**
     * To faciliate branch label determination, we use a fake insert technique
     * we first insert a fake instruction, and replace it when we know where the label is
     * We also use an unordered_map to keep track of the insertions
     * its contents will be like <current label, {wasm_instructions.size(), BranchType}>
     */
    int64_t cur_location = wasm_instructions.size();
    cout << format("       {} {}", (branchType.withLink ? "bl" : "b" + (branchType.withCondition ? "." + cond_str_map.at(branchType.cond) : "")),
                   label)
         << endl;
    wasm_instructions += "FFFFFFFF"; // fake insert branch instruction
    fake_insert_map.insert({label, {cur_location, branchType}});
  }
  void insertLabel(string label) {
    cout << format("{}: ", label) << endl;
    label_map.insert({label, wasm_instructions.size()});
  }
  void fixUpfakeBranch() {
    /**
     * After processing codevec finished, call this function to fix fake branch instructions
     * it will replace fake instruction with real ones
     */
    for (const auto &x : fake_insert_map) {
      auto [origin_location, branch_type] = x.second;
      cout << "++++++++++++++++" << endl;
      cout << format("Fixing fake branch to {} at {}", x.first, origin_location) << endl;
      cout << "Before fix: " << wasm_instructions.substr(origin_location, 8) << endl;
      // cout << wasm_instructions << endl;
      int instructions_level_offset = (label_map[x.first] - origin_location) / 8;
      cout << "Instructions level offset (estimated): " << instructions_level_offset << endl;
      // todo: check correctness
      streambuf *old = cout.rdbuf();
      cout.rdbuf(0);
      if (branch_type.withCondition) {
        wasm_instructions.replace(origin_location, 8, encodeBranchCondition(instructions_level_offset, branch_type.cond));
      } else {
        wasm_instructions.replace(origin_location, 8, encodeBranch(instructions_level_offset, branch_type.withLink));
      }
      cout.rdbuf(old);
      cout << "After fix: " << wasm_instructions.substr(origin_location, 8) << endl;
      // cout << wasm_instructions << endl;
    }
  }
  template <typename Func> auto getFunctionPointer(string full_instructions) -> Func {
    const size_t arraySize = full_instructions.length() / 2;
    auto charArray = make_unique<unsigned char[]>(arraySize); // 使用智能指针
    for (size_t i = 0; i < arraySize; ++i) {
      const string byteStr = full_instructions.substr(i * 2, 2);
      charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    Func instruction_set = nullptr;
    instruction_set = reinterpret_cast<Func>(mmap(nullptr, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (instruction_set == MAP_FAILED) {
      perror("mmap");
      exit(1);
    }
    memcpy(reinterpret_cast<void *>(instruction_set), charArray.get(), arraySize);
    __builtin___clear_cache(reinterpret_cast<char *>(instruction_set), reinterpret_cast<char *>(instruction_set) + arraySize);
    return instruction_set;
  }
  int64_t executeWasmInstr() {
    /**
     *
     * Allocate memory with execute permission
     * And load machine code into that
     *
     */
    // Warn: Append pre wasm_instructions here
    // string full_instructions = pre_instructions_for_param_loading + encodeBranch(1) + wasm_instructions;
    string full_instructions = pre_instructions_for_param_loading + wasm_instructions;
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
    auto instruction_set = getFunctionPointer<int64_t (*)(void *)>(full_instructions);
    void *buffer = malloc(1024);
    // !不需要做任何传参，因为参数已经放在寄存器里啦
    int64_t ans = instruction_set(buffer);
    auto return_code = *reinterpret_cast<int16_t *>(buffer);
    cout << "Return code is: " << return_code << endl;
    free(buffer);
    // munmap(reinterpret_cast<void *>(instruction_set), full_instructions); // Can't unmap here, TODO: will have memory leak here
    // WARN: reset things, very important if we want to call it again!
    resetAfterExecution();
    if (return_code) {
      throw string("Wasm trapped");
    }
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
    string load_param_instr = encodeLoadStoreImm(regtype, LDR, 11, 31, stack_offset);
    // var[i] -> x/w11
    string store_to_stack_instr = encodeLoadStoreImm(regtype, STR, 11, 31, wasm_stack_pointer);
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
    string store_to_stack_instr = encodeLoadStoreImm(regtype, LDR, 11, 31, wasm_stack_pointer);
    string reg_to_mem_instr = encodeLoadStoreImm(regtype, STR, 11, 31, stack_offset);
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
            string load_to_reg_instr = WrapperEncodeMovInt32(11, value);
            // cout << format("Emit: mov {}, w11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
            string store_to_stack_instr = encodeLoadStoreImm(W_REG, STR, 11, 31, wasm_stack_pointer);
            constructFullinstr(load_to_reg_instr + store_to_stack_instr);
          } else if (typeInfo == 'l') {
            cout << format("i64.const {}", value) << endl;
            string load_to_reg_instr = WrapperEncodeMovInt64(11, value);
            // cout << format("Emit: mov {}, x11 | {}", value, convertEndian(load_to_reg_instr)) << endl;
            string store_to_stack_instr = encodeLoadStoreImm(X_REG, STR, 11, 31, wasm_stack_pointer);
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
    string load_second_param_instr = encodeLoadStoreImm(regtype, LDR, 11, 31, wasm_stack_pointer);
    wasm_stack_pointer += 8;
    // r12 = a
    string load_first_param_instr = encodeLoadStoreImm(regtype, LDR, 12, 31, wasm_stack_pointer);

    constructFullinstr(load_first_param_instr + load_second_param_instr);
    // r11 = a op b
    string arith_instr;
    string check_div_instr;
    string branch_equal_zero_instr;
    switch (opType) {
    case '+':
      wasm_instructions += encodeAddSubShift(false, regtype, 11, 12, 11);
      break;
    case '-':
      wasm_instructions += encodeAddSubShift(true, regtype, 11, 12, 11);
      break;
    case '*':
      wasm_instructions += encodeMul(regtype, 11, 12, 11);
      break;
    case '/':
      wasm_instructions += encodeCompareImm(regtype, 11, 0);                                      // cmp b, #0
      fakeInsertBranch("preparelongjmp", BranchType(true, false, reverse_cond_str_map.at("eq"))); // checks for division by zero

      // if b = -1, checks a
      if (regtype == X_REG) {
        wasm_instructions += WrapperEncodeMovInt64(5, -1);
        wasm_instructions += encodeCompareShift(X_REG, 11, 5);
        fakeInsertBranch("normal_div", BranchType(true, false, reverse_cond_str_map.at("ne"))); // if b != -1, goto normal div

        wasm_instructions += WrapperEncodeMovInt64(5, INT64_MIN);
        wasm_instructions += encodeCompareShift(X_REG, 12, 5);
        fakeInsertBranch("normal_div", BranchType(true, false, reverse_cond_str_map.at("ne"))); // if a != INT64_MIN, goto normal div
      } else {
        wasm_instructions += WrapperEncodeMovInt32(5, -1);
        wasm_instructions += encodeCompareShift(W_REG, 11, 5);
        fakeInsertBranch("normal_div", BranchType(true, false, reverse_cond_str_map.at("ne"))); // if b != -1, goto normal div

        wasm_instructions += WrapperEncodeMovInt32(5, INT32_MIN);
        wasm_instructions += encodeCompareShift(W_REG, 12, 5);
        fakeInsertBranch("normal_div", BranchType(true, false, reverse_cond_str_map.at("ne"))); // if a != INT32_MIN, goto normal div
      }
      fakeInsertBranch("preparelongjmp", BranchType()); // this means a=INT32_MIN and b=-1, goto raise exception int overflow

      insertLabel("normal_div");
      wasm_instructions += encodeDiv(regtype, isSigned, 11, 12, 11);

      break;
    default:
      throw "Unknown arithmetic operator";
      break;
    }
    wasm_instructions += encodeLoadStoreImm(regtype, STR, 11, 31, wasm_stack_pointer); // store_to_stack
    wasm_stack_pointer -= 8;                                                           // decrease wasm stack after push
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
  void jiting_wasm_code(int i) {
    cout << "--- JITing wasm code ---" << endl;
    wasm_stack_pointer = wasm_stack_end_location - 8; // WARN!!! VERY IMPORTANT NOT TO USE THE END LOCATION OR IT WILL OVERWRITE X29
    // cout << format("*Current wasm stack pointer is: {}", wasm_stack_pointer) << endl;
    while (i < code_vec.size()) {
      /**
       * WebAssembly Opcodes
       * https://pengowray.github.io/wasm-ops/
       */
      if (code_vec[i] == "0f") { // ret
        break;                   // todo: maybe need further handling here??
        // can't tell the difference between ret and end yet.
        restoreStack();
        emitRet();
        ++i;
      } else if (code_vec[i] == "0b") { // end
        break;                          // todo: maybe need further handling here??
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
      // cout << format("*Current wasm stack pointer is: {}", wasm_stack_pointer) << endl;
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
            if (value == 0) {
              // cout << "! Division by zero in wasm code" << endl;
            }
            return wasm_type(0);
            // disabled for now to check arm64 trap!!
            // throw std::runtime_error("Division by zero");
          },
          b);
      return a / b;
    };
  }
  // data section
  int stack_size = 0;
  int param_stack_start_location = 0;
  int param_stack_end_location = 0;
  int local_stack_start_location = 0;
  int local_stack_end_location = 0;
  int wasm_stack_start_location = 0;
  int wasm_stack_end_location = 0;
  int wasm_stack_pointer = 0;
  int type;
  u_int64_t local_var_declare_count = 0;

  string wasm_instructions;
  string pre_instructions_for_param_loading;

  vector<string> code_vec;
  vector<wasm_type> local_data;
  vector<wasm_type> param_data;
  vector<wasm_type> result_data;
  vector<wasm_type> stack;

  map<char, ArithOperation> operations_map;
  map<pair<TypeCategory, int>, int> vecToStack;        // {TypeCategory::PARAM, 0} : 0x4
  map<pair<TypeCategory, int>, RegType> regTypeGetter; // {TypeCategory::PARAM, 0}: LDR32
  map<int, pair<TypeCategory, int>> stackToVec;        // 0x4 : {TypeCategory::PARAM: 0}
  unordered_multimap<string, pair<int64_t, BranchType>> fake_insert_map;
  unordered_map<string, int64_t> label_map;
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