(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (result i64)))
  (type (;2;) (func (param i32) (result i32)))
  (type (;3;) (func (param i64) (result i64)))
  (type (;4;) (func (param i32 i64) (result i64)))
  (type (;5;) (func (param i64 i64) (result i64)))
  (type (;6;) (func (param i64) (result i32)))
  (func (;0;) (type 0) (result i32)
    i32.const 306)
  (func (;1;) (type 1) (result i64)
    i64.const 356)
  (func (;2;) (type 2) (param i32) (result i32)
    local.get 0)
  (func (;3;) (type 3) (param i64) (result i64)
    local.get 0)
  (func (;4;) (type 0) (result i32)
    call 0)
  (func (;5;) (type 1) (result i64)
    call 1)
  (func (;6;) (type 0) (result i32)
    i32.const 32
    call 2)
  (func (;7;) (type 1) (result i64)
    i64.const 64
    call 3)
  (func (;8;) (type 1) (result i64)
    i32.const 32
    i64.const 64
    call 9)
  (func (;9;) (type 4) (param i32 i64) (result i64)
    local.get 1)
  (func (;10;) (type 5) (param i64 i64) (result i64)
    local.get 0
    i64.eqz
    if (result i64)  ;; label = @1
      local.get 1
    else
      local.get 0
      i64.const 1
      i64.sub
      local.get 0
      local.get 1
      i64.mul
      call 10
    end)
  (func (;11;) (type 3) (param i64) (result i64)
    local.get 0
    i64.const 1
    i64.le_u
    if (result i64)  ;; label = @1
      i64.const 1
    else
      local.get 0
      i64.const 2
      i64.sub
      call 11
      local.get 0
      i64.const 1
      i64.sub
      call 11
      i64.add
    end)
  (func (;12;) (type 6) (param i64) (result i32)
    local.get 0
    i64.eqz
    if (result i32)  ;; label = @1
      i32.const 44
    else
      local.get 0
      i64.const 1
      i64.sub
      call 13
    end)
  (func (;13;) (type 6) (param i64) (result i32)
    local.get 0
    i64.eqz
    if (result i32)  ;; label = @1
      i32.const 99
    else
      local.get 0
      i64.const 1
      i64.sub
      call 12
    end)
  (export "type-i32" (func 4))
  (export "type-i64" (func 5))
  (export "type-first-i32" (func 6))
  (export "type-first-i64" (func 7))
  (export "type-second-i64" (func 8))
  (export "fac-acc" (func 10))
  (export "fib" (func 11))
  (export "even" (func 12))
  (export "odd" (func 13)))
