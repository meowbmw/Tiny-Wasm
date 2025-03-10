(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func))
  (type (;2;) (func (param i32 i32 i32) (result i32)))
  (type (;3;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (result i32)
    block (result i32)  ;; label = @1
      i32.const 0
      i32.load
      br 0 (;@1;)
    end)
  (func (;1;) (type 1)
    block  ;; label = @1
      i32.const 0
      i32.load
      br_if 0 (;@1;)
    end)
  (func (;2;) (type 0) (result i32)
    block (result i32)  ;; label = @1
      i32.const 0
      i32.load
      i32.const 1
      br_if 0 (;@1;)
      drop
      i32.const 7
    end)
  (func (;3;) (type 0) (result i32)
    block (result i32)  ;; label = @1
      i32.const 6
      i32.const 0
      i32.load
      br_if 0 (;@1;)
      drop
      i32.const 7
    end)
  (func (;4;) (type 0) (result i32)
    i32.const 0
    i32.load
    return)
  (func (;5;) (type 0) (result i32)
    i32.const 0
    i32.load
    if (result i32)  ;; label = @1
      i32.const 0
    else
      i32.const 1
    end)
  (func (;6;) (type 0) (result i32)
    i32.const 1
    if (result i32)  ;; label = @1
      i32.const 0
      i32.load
    else
      i32.const 0
    end)
  (func (;7;) (type 0) (result i32)
    i32.const 0
    if (result i32)  ;; label = @1
      i32.const 0
    else
      i32.const 0
      i32.load
    end)
  (func (;8;) (type 0) (result i32)
    i32.const 0
    i32.load
    i32.const 2
    i32.const 3
    call 11)
  (func (;9;) (type 0) (result i32)
    i32.const 1
    i32.const 0
    i32.load
    i32.const 3
    call 11)
  (func (;10;) (type 0) (result i32)
    i32.const 1
    i32.const 2
    i32.const 0
    i32.load
    call 11)
  (func (;11;) (type 2) (param i32 i32 i32) (result i32)
    i32.const -1)
  (func (;12;) (type 1)
    (local i32)
    i32.const 0
    i32.load
    local.set 0)
  (func (;13;) (type 0) (result i32)
    (local i32)
    i32.const 0
    i32.load
    local.tee 0)
  (func (;14;) (type 1)
    (local i32)
    i32.const 0
    i32.load
    global.set 0)
  (func (;15;) (type 0) (result i32)
    i32.const 0
    i32.load
    i32.load)
  (func (;16;) (type 0) (result i32)
    i32.const 0
    i32.load
    i32.load8_s)
  (func (;17;) (type 1)
    i32.const 0
    i32.load
    i32.const 7
    i32.store)
  (func (;18;) (type 1)
    i32.const 2
    i32.const 0
    i32.load
    i32.store)
  (func (;19;) (type 1)
    i32.const 0
    i32.load8_s
    i32.const 7
    i32.store8)
  (func (;20;) (type 1)
    i32.const 2
    i32.const 0
    i32.load
    i32.store16)
  (func (;21;) (type 0) (result i32)
    i32.const 100
    i32.load
    i32.clz)
  (func (;22;) (type 0) (result i32)
    i32.const 100
    i32.load
    i32.const 10
    i32.add)
  (func (;23;) (type 0) (result i32)
    i32.const 10
    i32.const 100
    i32.load
    i32.sub)
  (func (;24;) (type 0) (result i32)
    i32.const 100
    i32.load
    i32.eqz)
  (func (;25;) (type 0) (result i32)
    i32.const 100
    i32.load
    i32.const 10
    i32.le_s)
  (func (;26;) (type 0) (result i32)
    i32.const 10
    i32.const 100
    i32.load
    i32.ne)
  (func (;27;) (type 0) (result i32)
    i32.const 100
    i32.load
    memory.grow)
  (func (;28;) (type 3) (param i32) (result i32)
    local.get 0
    i32.load
    memory.grow)
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
  (export "as-call-first" (func 8))
  (export "as-call-mid" (func 9))
  (export "as-call-last" (func 10))
  (export "as-local.set-value" (func 12))
  (export "as-local.tee-value" (func 13))
  (export "as-global.set-value" (func 14))
  (export "as-load-address" (func 15))
  (export "as-loadN-address" (func 16))
  (export "as-store-address" (func 17))
  (export "as-store-value" (func 18))
  (export "as-storeN-address" (func 19))
  (export "as-storeN-value" (func 20))
  (export "as-unary-operand" (func 21))
  (export "as-binary-left" (func 22))
  (export "as-binary-right" (func 23))
  (export "as-test-operand" (func 24))
  (export "as-compare-left" (func 25))
  (export "as-compare-right" (func 26))
  (export "as-memory.grow-size" (func 27))
  (export "address-as-param" (func 28)))
