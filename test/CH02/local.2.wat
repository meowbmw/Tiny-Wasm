(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (result i64)))
  (type (;2;) (func (param i32) (result i32)))
  (type (;3;) (func (param i64) (result i64)))
  (type (;4;) (func))
  (func (;0;) (type 0) (result i32)
    (local i32)
    i32.const 1
    local.tee 0)
  (func (;1;) (type 1) (result i64)
    (local i64)
    i64.const 1
    local.tee 0)
  (func (;2;) (type 2) (param i32) (result i32)
    i32.const 10
    local.tee 0)
  (func (;3;) (type 3) (param i64) (result i64)
    i64.const 11
    local.tee 0)
  (func (;4;) (type 4)
    (local i32)
    i32.const 1
    local.tee 0
    local.set 0)
  (func (;5;) (type 2) (param i32) (result i32)
    i32.const 1
    local.tee 0
    local.tee 0)
  (export "type-local-i32" (func 0))
  (export "type-local-i64" (func 1))
  (export "type-param-i32" (func 2))
  (export "type-param-i64" (func 3))
  (export "as-local.set-value" (func 4))
  (export "as-local.tee-value" (func 5)))