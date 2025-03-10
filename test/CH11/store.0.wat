(module
  (type (;0;) (func))
  (type (;1;) (func (param i32 i32)))
  (func (;0;) (type 0)
    block  ;; label = @1
      i32.const 0
      i32.const 1
      i32.store
    end)
  (func (;1;) (type 0)
    loop  ;; label = @1
      i32.const 0
      i32.const 1
      i32.store
    end)
  (func (;2;) (type 0)
    block  ;; label = @1
      i32.const 0
      i32.const 1
      i32.store
      br 0 (;@1;)
    end)
  (func (;3;) (type 0)
    block  ;; label = @1
      i32.const 0
      i32.const 1
      i32.store
      i32.const 1
      br_if 0 (;@1;)
    end)
  (func (;4;) (type 0)
    block  ;; label = @1
      i32.const 6
      i32.const 0
      i32.const 1
      i32.store
      br_if 0 (;@1;)
    end)
  (func (;5;) (type 0)
    i32.const 0
    i32.const 1
    i32.store
    return)
  (func (;6;) (type 0)
    i32.const 1
    if  ;; label = @1
      i32.const 0
      i32.const 1
      i32.store
    end)
  (func (;7;) (type 0)
    i32.const 0
    if  ;; label = @1
    else
      i32.const 0
      i32.const 1
      i32.store
    end)
  (func (;8;) (type 1) (param i32 i32)
    local.get 0
    local.get 1
    i32.store)
  (memory (;0;) 1)
  (export "as-block-value" (func 0))
  (export "as-loop-value" (func 1))
  (export "as-br-value" (func 2))
  (export "as-br_if-value" (func 3))
  (export "as-br_if-value-cond" (func 4))
  (export "as-return-value" (func 5))
  (export "as-if-then" (func 6))
  (export "as-if-else" (func 7))
  (export "address-as-param" (func 8)))
