(module
  (type (;0;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (param i32) (result i32)
    local.get 0
    if  ;; label = @1
      nop
    end
    local.get 0
    if  ;; label = @1
      nop
    else
      nop
    end
    local.get 0
    if (result i32)  ;; label = @1
      i32.const 7
    else
      i32.const 8
    end)
  (func (;1;) (type 0) (param i32) (result i32)
    local.get 0
    if (result i32)  ;; label = @1
      i32.const 1
    else
      i32.const 0
    end
    return)
  (func (;2;) (type 0) (param i32) (result i32)
    (local i32)
    local.get 0
    if (result i32)  ;; label = @1
      i32.const 1
    else
      i32.const 0
    end
    local.set 0
    local.get 0)
  (func (;3;) (type 0) (param i32) (result i32)
    local.get 0
    if (result i32)  ;; label = @1
      i32.const 1
    else
      i32.const 0
    end
    local.tee 0)
  (export "singular" (func 0))
  (export "as-return-value" (func 1))
  (export "as-local.set-value" (func 2))
  (export "as-local.tee-value" (func 3)))
