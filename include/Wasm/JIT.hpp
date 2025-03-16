#pragma once
#include "WasmFunction.hpp"

void WasmFunction::jiting_wasm_code(int i) {
  cout << "--- JITing wasm code ---" << endl;
  // This instruction is necessary for "end" to pop off control stack
  control_flow_stack.push_back(controlFlowElement(
      "Function end", result_data)); // TODO: this might need to be called on every function enter, currently it is only executed once.
  // backup x30, i.e. [sp, stacksize-8] = x30
  wasm_instructions += encodeLoadStoreImm(X_REG, STR, 30, 31, stack_size - 8);
  while (i < code_vec.size()) {
    /**
     * WebAssembly Opcodes
     * https://pengowray.github.io/wasm-ops/
     */
    if (code_vec[i] == "01") {
      // nop
      wasm_instructions += encodeNop();
      i += 1;
    } else if (code_vec[i] == "04") { // if
      // todo: currently don't support multi-value return
      // we assume at most one return can occur, or simply none return
      emitIfOp(i + 1);
      i += 2;
    } else if (code_vec[i] == "05") { // else
      emitElseOp();
      i += 1;
    } else if (code_vec[i] == "0f") { // ret
      emitReturnOp();
      i += 1;
    } else if (code_vec[i] == "0b") { // end
      emitEndOp();
      i += 1;
    } else if (code_vec[i] == "02") { // block
      emitBlock(i);
      i += 2;
    } else if (code_vec[i] == "03") { // loop
      emitLoop(i);
      i += 2;
    } else if (code_vec[i] == "0c") { // br
      emitBr(i);
      i += 2;
    } else if (code_vec[i] == "0d") { // br_if
      emitBr_if(i);
      i += 2;
    } else if (code_vec[i] == "1a") { // drop
      emitDrop();
      i += 1;
    } else if (code_vec[i] == "10") { // call
      auto [function_index, bytesRead] = decode_uleb128_from_vec(code_vec, i + 1);
      emitCall(function_index);
      i += bytesRead + 1;
    } else if (code_vec[i] == "11") { // call_indirect
      auto [type_index, bytesRead_1] = decode_uleb128_from_vec(code_vec, i + 1);
      i += bytesRead_1 + 1;
      auto [table_index, bytesRead_2] = decode_uleb128_from_vec(code_vec, i);
      i += bytesRead_2;
      emitCallIndirect(type_index, table_index);
    }
    // local
    else if (code_vec[i] == "20") { // local.get
      auto [var_index, bytes_read] = decode_uleb128_from_vec(code_vec, i + 1);
      commonLocalOp(var_index, "get");
      i += bytes_read + 1;
    } else if (code_vec[i] == "21") { // local.set
      auto [var_index, bytes_read] = decode_uleb128_from_vec(code_vec, i + 1);
      commonLocalOp(var_index, "set");
      i += bytes_read + 1;
    } else if (code_vec[i] == "22") { // local.tee
      auto [var_index, bytes_read] = decode_uleb128_from_vec(code_vec, i + 1);
      commonLocalOp(var_index, "tee");
      i += bytes_read + 1;
    }
    // global
    else if (code_vec[i] == "23") { // global.get
      auto [var_index, bytes_read] = decode_uleb128_from_vec(code_vec, i + 1);
      emitGlobalGet(var_index);
      i += bytes_read + 1;
    } else if (code_vec[i] == "24") { // global.set
      auto [var_index, bytes_read] = decode_uleb128_from_vec(code_vec, i + 1);
      emitGlobalSet(var_index);
      i += bytes_read + 1;
    }
    // load
    else if (code_vec[i] == "28") { // i32.load
      commonLoadStoreOp(i, W_REG, LDR, DataWidth::doubleword, ZeroExtend);
    } else if (code_vec[i] == "29") { // i64.load
      commonLoadStoreOp(i, X_REG, LDR, DataWidth::quadword, ZeroExtend);
    }
    // store
    else if (code_vec[i] == "36") { // i32.store
      commonLoadStoreOp(i, W_REG, STR, DataWidth::doubleword, ZeroExtend);
    } else if (code_vec[i] == "37") { // i64.store
      commonLoadStoreOp(i, X_REG, STR, DataWidth::quadword, ZeroExtend);
    }
    // extend load, signed
    else if (code_vec[i] == "2c") { // i32.load8_s

    } else if (code_vec[i] == "2e") { // i32.load16_s

    } else if (code_vec[i] == "30") { // i64.load8_s

    } else if (code_vec[i] == "32") { // i64.load16_s

    } else if (code_vec[i] == "34") { // i64.load32_s

    }
    // extend load, unsigned
    else if (code_vec[i] == "2d") { // i32.load8_u
      commonLoadStoreOp(i, W_REG, LDR, DataWidth::byte, ZeroExtend);
    } else if (code_vec[i] == "2f") { // i32.load16_u
      commonLoadStoreOp(i, W_REG, LDR, DataWidth::word, ZeroExtend);
    } else if (code_vec[i] == "31") { // i64.load8_u
      commonLoadStoreOp(i, X_REG, LDR, DataWidth::byte, ZeroExtend);
    } else if (code_vec[i] == "33") { // i64.load16_u
      commonLoadStoreOp(i, X_REG, LDR, DataWidth::word, ZeroExtend);
    } else if (code_vec[i] == "35") { // i64.load32_u
      commonLoadStoreOp(i, X_REG, LDR, DataWidth::doubleword, ZeroExtend);
    }
    // wrapping store
    else if (code_vec[i] == "2c") { // i32.store8

    } else if (code_vec[i] == "2e") { // i32.store16

    } else if (code_vec[i] == "30") { // i64.store8

    } else if (code_vec[i] == "32") { // i64.store16

    } else if (code_vec[i] == "34") { // i64.store32

    }
    // grow
    else if (code_vec[i] == "40") { // memory.grow
      emitMemoryGrow();
      i += 2; // there should be a memidx, but it is not used yet. So i += 2
    }
    // get current memory size
    else if (code_vec[i] == "3f") { // memory.size
      emitMemorySize();
      i += 2; // there should be a memidx, but it is not used yet. So i += 2
    }
    // const
    else if (code_vec[i] == "41") { // i32.const
      auto [value, bytesRead] = decode_sleb128_from_vec(code_vec, i + 1);
      wasm_type elem = static_cast<int32_t>(value);
      emitConst(elem);
      i += bytesRead + 1;
    } else if (code_vec[i] == "42") { // i64.const
      auto [value, bytesRead] = decode_sleb128_from_vec(code_vec, i + 1);
      wasm_type elem = static_cast<int64_t>(value);
      emitConst(elem);
      i += bytesRead + 1;
    } else if (code_vec[i] == "43") { // f32.const
      wasm_type elem = hexToFloat(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4]);
      emitConst(elem);
      i += 5;
    } else if (code_vec[i] == "44") { // f64.const
      wasm_type elem =
          hexToDouble(code_vec[i + 1] + code_vec[i + 2] + code_vec[i + 3] + code_vec[i + 4] + code_vec[i + 5] + code_vec[i + 6] + code_vec[i + 7]);
      emitConst(elem);
      i += 9;
    }
    // integer comparsion
    else if (code_vec[i] == "46") { // i32.eq
      emitCompareOp(W_REG, "eq");
      i += 1;
    } else if (code_vec[i] == "51") { // i64.eq
      emitCompareOp(X_REG, "eq");
      i += 1;
    } else if (code_vec[i] == "47") { // i32.ne
      emitCompareOp(W_REG, "ne");
      i += 1;
    } else if (code_vec[i] == "52") { // i64.ne
      emitCompareOp(X_REG, "ne");
      i += 1;
    } else if (code_vec[i] == "48") { // i32.lt_s
      emitCompareOp(W_REG, "lt");
      i += 1;
    } else if (code_vec[i] == "49") { // i32.lt_u
      emitCompareOp(W_REG, "cc");
      i += 1;
    } else if (code_vec[i] == "4a") { // i32.gt_s
      emitCompareOp(W_REG, "gt");
      i += 1;
    } else if (code_vec[i] == "4b") { // i32.gt_u
      emitCompareOp(W_REG, "hi");
      i += 1;
    } else if (code_vec[i] == "4c") { // i32.le_s
      emitCompareOp(W_REG, "le");
      i += 1;
    } else if (code_vec[i] == "4d") { // i32.le_u
      emitCompareOp(W_REG, "ls");
      i += 1;
    } else if (code_vec[i] == "4e") { // i32.ge_s
      emitCompareOp(W_REG, "ge");
      i += 1;
    } else if (code_vec[i] == "4f") { // i32.ge_u
      emitCompareOp(W_REG, "cs");
      i += 1;
    } else if (code_vec[i] == "53") { // i64.lt_s
      emitCompareOp(X_REG, "lt");
      i += 1;
    } else if (code_vec[i] == "54") { // i64.lt_u
      emitCompareOp(X_REG, "cc");
      i += 1;
    } else if (code_vec[i] == "55") { // i64.gt_s
      emitCompareOp(X_REG, "gt");
      i += 1;
    } else if (code_vec[i] == "56") { // i64.gt_u
      emitCompareOp(X_REG, "hi");
      i += 1;
    } else if (code_vec[i] == "57") { // i64.le_s
      emitCompareOp(X_REG, "le");
      i += 1;
    } else if (code_vec[i] == "58") { // i64.le_u
      emitCompareOp(X_REG, "ls");
      i += 1;
    } else if (code_vec[i] == "59") { // i64.ge_s
      emitCompareOp(X_REG, "ge");
      i += 1;
    } else if (code_vec[i] == "5a") { // i64.ge_u
      emitCompareOp(X_REG, "cs");
      i += 1;
    }
    // arithmetic
    else if (code_vec[i] == "6a") { // i32.add
      emitArithOp('i', '+');
      i += 1;
    } else if (code_vec[i] == "6b") { // i32.sub
      emitArithOp('i', '-');
      i += 1;
    } else if (code_vec[i] == "6c") { // i32.mul
      emitArithOp('i', '*');
      i += 1;
    } else if (code_vec[i] == "6d") { // i32.div_s
      emitArithOp('i', '/', true);
      i += 1;
    } else if (code_vec[i] == "6e") { // i32.div_u
      emitArithOp('i', '/', false);
      i += 1;
    } else if (code_vec[i] == "7c") { // i64.add
      emitArithOp('l', '+');
      i += 1;
    } else if (code_vec[i] == "7d") { // i64.sub
      emitArithOp('l', '-');
      i += 1;
    } else if (code_vec[i] == "7e") { // i64.mul
      emitArithOp('l', '*');
      i += 1;
    } else if (code_vec[i] == "7f") { // i64.div_s
      emitArithOp('l', '/', true);
      i += 1;
    } else if (code_vec[i] == "80") { // i64.div_u
      emitArithOp('l', '/', false);
      i += 1;
    }
    // other operations
    else if (code_vec[i] == "45") { // i32.eqz
      emitEqz(W_REG);
      i += 1;
    } else if (code_vec[i] == "50") { // i64.eqz
      emitEqz(X_REG);
      i += 1;
    } else if (code_vec[i] == "68") { // i32.ctz
      emitCtz(W_REG);
      i += 1;
    } else if (code_vec[i] == "7a") { // i64.ctz
      emitCtz(X_REG);
      i += 1;
    }
  }
  // restore x30, i.e. x30 = [sp, stacksize-8]
  wasm_instructions += encodeLoadStoreImm(X_REG, LDR, 30, 31, stack_size - 8);
}
