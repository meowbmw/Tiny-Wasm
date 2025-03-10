(module
  (type (;0;) (func (param i32 i32 i32) (result i32)))
  (type (;1;) (func (result i32)))
  (type (;2;) (func))
  (func (;0;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 0
      memory.grow
      br 0 (;@1;)
    end)
  (func (;1;) (type 2)
    block  ;; label = @1
      i32.const 0
      memory.grow
      br_if 0 (;@1;)
    end)
  (func (;2;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 0
      memory.grow
      i32.const 1
      br_if 0 (;@1;)
      drop
      i32.const 7
    end)
  (func (;3;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 6
      i32.const 0
      memory.grow
      br_if 0 (;@1;)
      drop
      i32.const 7
    end)
  (func (;4;) (type 1) (result i32)
    i32.const 0
    memory.grow
    return)
  (func (;5;) (type 1) (result i32)
    i32.const 0
    memory.grow
    if (result i32)  ;; label = @1
      i32.const 0
    else
      i32.const 1
    end)
  (func (;6;) (type 1) (result i32)
    i32.const 1
    if (result i32)  ;; label = @1
      i32.const 0
      memory.grow
    else
      i32.const 0
    end)
  (func (;7;) (type 1) (result i32)
    i32.const 0
    if (result i32)  ;; label = @1
      i32.const 0
    else
      i32.const 0
      memory.grow
    end)
  (func (;8;) (type 0) (param i32 i32 i32) (result i32)
    i32.const -1)
  (func (;9;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.const 2
    i32.const 3
    call 8)
  (func (;10;) (type 1) (result i32)
    i32.const 1
    i32.const 0
    memory.grow
    i32.const 3
    call 8)
  (func (;11;) (type 1) (result i32)
    i32.const 1
    i32.const 2
    i32.const 0
    memory.grow
    call 8)
  (func (;12;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.const 2
    i32.const 3
    i32.const 0
    call_indirect (type 0))
  (func (;13;) (type 1) (result i32)
    i32.const 1
    i32.const 0
    memory.grow
    i32.const 3
    i32.const 0
    call_indirect (type 0))
  (func (;14;) (type 1) (result i32)
    i32.const 1
    i32.const 2
    i32.const 0
    memory.grow
    i32.const 0
    call_indirect (type 0))
  (func (;15;) (type 1) (result i32)
    i32.const 1
    i32.const 2
    i32.const 3
    i32.const 0
    memory.grow
    call_indirect (type 0))
  (func (;16;) (type 2)
    (local i32)
    i32.const 0
    memory.grow
    local.set 0)
  (func (;17;) (type 1) (result i32)
    (local i32)
    i32.const 0
    memory.grow
    local.tee 0)
  (func (;18;) (type 2)
    (local i32)
    i32.const 0
    memory.grow
    global.set 0)
  (func (;19;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.load)
  (func (;20;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.load8_s)
  (func (;21;) (type 2)
    i32.const 0
    memory.grow
    i32.const 7
    i32.store)
  (func (;22;) (type 2)
    i32.const 2
    i32.const 0
    memory.grow
    i32.store)
  (func (;23;) (type 2)
    i32.const 0
    memory.grow
    i32.const 7
    i32.store8)
  (func (;24;) (type 2)
    i32.const 2
    i32.const 0
    memory.grow
    i32.store16)
  (func (;25;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.clz)
  (func (;26;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.const 10
    i32.add)
  (func (;27;) (type 1) (result i32)
    i32.const 10
    i32.const 0
    memory.grow
    i32.sub)
  (func (;28;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.eqz)
  (func (;29;) (type 1) (result i32)
    i32.const 0
    memory.grow
    i32.const 10
    i32.le_s)
  (func (;30;) (type 1) (result i32)
    i32.const 10
    i32.const 0
    memory.grow
    i32.ne)
  (func (;31;) (type 1) (result i32)
    i32.const 0
    memory.grow
    memory.grow)
  (table (;0;) 1 1 funcref)
  (memory (;0;) 1)
  (global (;0;) (mut i32) (i32.const 0))
  (export "as-br-value" (func 0))
  (export "as-br_if-cond" (func 1))
  (export "as-br_if-value" (func 2))
  (export "as-br_if-value-cond" (func 3))
  (export "as-return-value" (func 4))
  (export "as-if-cond" (func 5))
  (export "as-if-then" (func 6))
  (export "as-if-else" (func 7))
  (export "as-call-first" (func 9))
  (export "as-call-mid" (func 10))
  (export "as-call-last" (func 11))
  (export "as-call_indirect-first" (func 12))
  (export "as-call_indirect-mid" (func 13))
  (export "as-call_indirect-last" (func 14))
  (export "as-call_indirect-index" (func 15))
  (export "as-local.set-value" (func 16))
  (export "as-local.tee-value" (func 17))
  (export "as-global.set-value" (func 18))
  (export "as-load-address" (func 19))
  (export "as-loadN-address" (func 20))
  (export "as-store-address" (func 21))
  (export "as-store-value" (func 22))
  (export "as-storeN-address" (func 23))
  (export "as-storeN-value" (func 24))
  (export "as-unary-operand" (func 25))
  (export "as-binary-left" (func 26))
  (export "as-binary-right" (func 27))
  (export "as-test-operand" (func 28))
  (export "as-compare-left" (func 29))
  (export "as-compare-right" (func 30))
  (export "as-memory.grow-size" (func 31))
  (elem (;0;) (i32.const 0) func 8))
