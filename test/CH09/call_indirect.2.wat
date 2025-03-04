(module
  (type (;0;) (func (param i32) (result i32)))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i32 i32) (result i32)))
  (func (;0;) (type 0) (param i32) (result i32)
    local.get 0
    i32.const 1
    i32.add)
  (func (;1;) (type 1) (param i32))
  (func (;2;) (type 2) (param i32 i32) (result i32)
    local.get 1
    local.get 0
    call_indirect (type 0))
  (table (;0;) 2 funcref)
  (export "table" (table 0))
  (export "call_indirect" (func 2))
  (elem (;0;) (i32.const 0) func 0 1))
