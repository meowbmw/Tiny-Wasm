(module
  (type (;0;) (func))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i64)))
  (type (;3;) (func (param i64 f32 f64 i32 i32)))
  (func (;0;) (type 0)
    (local i32)
    i32.const 0
    local.set 0)
  (func (;1;) (type 0)
    (local i64)
    i64.const 0
    local.set 0)
  (func (;2;) (type 1) (param i32)
    i32.const 10
    local.set 0)
  (func (;3;) (type 2) (param i64)
    i64.const 11
    local.set 0)
  (func (;4;) (type 3) (param i64 f32 f64 i32 i32)
    (local f32 i64 i64 f64)
    i64.const 0
    local.set 0
    f32.const 0x0p+0 (;=0;)
    local.set 1
    f64.const 0x0p+0 (;=0;)
    local.set 2
    i32.const 0
    local.set 3
    i32.const 0
    local.set 4
    f32.const 0x0p+0 (;=0;)
    local.set 5
    i64.const 0
    local.set 6
    i64.const 0
    local.set 7
    f64.const 0x0p+0 (;=0;)
    local.set 8)
  (export "type-local-i32" (func 0))
  (export "type-local-i64" (func 1))
  (export "type-param-i32" (func 2))
  (export "type-param-i64" (func 3))
  (export "type-mixed" (func 4)))
