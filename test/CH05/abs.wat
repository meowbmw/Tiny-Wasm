(module
  (type (;0;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (param i32) (result i32)
    local.get 0
    i32.const 0
    i32.lt_s
    if (result i32)  ;; label = @1
      i32.const 0
      local.get 0
      i32.sub
    else
      local.get 0
    end)
  (export "abs" (func 0)))
