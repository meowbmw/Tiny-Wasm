(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (result i64)))
  (type (;2;) (func (param i32)))
  (type (;3;) (func (param i64)))
  (func (;0;) (type 0) (result i32)
    global.get 0)
  (func (;1;) (type 1) (result i64)
    global.get 1)
  (func (;2;) (type 0) (result i32)
    global.get 2)
  (func (;3;) (type 1) (result i64)
    global.get 3)
  (func (;4;) (type 2) (param i32)
    local.get 0
    global.set 2)
  (func (;5;) (type 3) (param i64)
    local.get 0
    global.set 3)
  (func (;6;) (type 0) (result i32)
    i32.const 1
    if (result i32)  ;; label = @1
      global.get 2
    else
      i32.const 2
    end)
  (func (;7;) (type 0) (result i32)
    i32.const 0
    if (result i32)  ;; label = @1
      i32.const 2
    else
      global.get 2
    end)
  (func (;8;) (type 0) (result i32)
    block (result i32)  ;; label = @1
      global.get 2
      i32.const 2
      br_if 0 (;@1;)
      i32.const 3
      return
    end)
  (func (;9;) (type 0) (result i32)
    block (result i32)  ;; label = @1
      i32.const 2
      global.get 2
      br_if 0 (;@1;)
      i32.const 3
      return
    end)
  (global (;0;) i32 (i32.const -2))
  (global (;1;) i64 (i64.const -5))
  (global (;2;) (mut i32) (i32.const -12))
  (global (;3;) (mut i64) (i64.const -15))
  (export "get-a" (func 0))
  (export "get-b" (func 1))
  (export "get-x" (func 2))
  (export "get-y" (func 3))
  (export "set-x" (func 4))
  (export "set-y" (func 5))
  (export "as-if-then" (func 6))
  (export "as-if-else" (func 7))
  (export "as-br_if-first" (func 8))
  (export "as-br_if-last" (func 9)))
