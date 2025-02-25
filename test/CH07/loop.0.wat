(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (param i64) (result i64)))
  (func (;0;) (type 0) (result i32)
    i32.const 1
    if (result i32)  ;; label = @1
      loop (result i32)  ;; label = @2
        i32.const 1
      end
    else
      i32.const 2
    end)
  (func (;1;) (type 0) (result i32)
    i32.const 1
    if (result i32)  ;; label = @1
      i32.const 2
    else
      loop (result i32)  ;; label = @2
        i32.const 1
      end
    end)
  (func (;2;) (type 0) (result i32)
    block (result i32)  ;; label = @1
      loop (result i32)  ;; label = @2
        i32.const 1
      end
      i32.const 2
      br_if 0 (;@1;)
    end)
  (func (;3;) (type 0) (result i32)
    block (result i32)  ;; label = @1
      i32.const 2
      loop (result i32)  ;; label = @2
        i32.const 1
      end
      br_if 0 (;@1;)
    end)
  (func (;4;) (type 0) (result i32)
    (local i32)
    block  ;; label = @1
      loop  ;; label = @2
        i32.const 1
        local.set 0
        local.get 0
        i32.const 3
        i32.mul
        local.set 0
        local.get 0
        i32.const 5
        i32.sub
        local.set 0
        local.get 0
        i32.const 7
        i32.mul
        local.set 0
        br 1 (;@1;)
        local.get 0
        i32.const 100
        i32.mul
        local.set 0
      end
    end
    local.get 0
    i32.const -14
    i32.eq)
  (func (;5;) (type 1) (param i64) (result i64)
    (local i64)
    i64.const 1
    local.set 1
    block  ;; label = @1
      loop  ;; label = @2
        local.get 0
        i64.eqz
        br_if 1 (;@1;)
        local.get 0
        local.get 1
        i64.mul
        local.set 1
        local.get 0
        i64.const 1
        i64.sub
        local.set 0
        br 0 (;@2;)
      end
    end
    local.get 1)
  (func (;6;) (type 1) (param i64) (result i64)
    (local i64 i64)
    i64.const 1
    local.set 1
    i64.const 2
    local.set 2
    block  ;; label = @1
      loop  ;; label = @2
        local.get 2
        local.get 0
        i64.gt_u
        br_if 1 (;@1;)
        local.get 1
        local.get 2
        i64.mul
        local.set 1
        local.get 2
        i64.const 1
        i64.add
        local.set 2
        br 0 (;@2;)
      end
    end
    local.get 1)
  (export "as-if-then" (func 0))
  (export "as-if-else" (func 1))
  (export "as-br_if-first" (func 2))
  (export "as-br_if-last" (func 3))
  (export "effects" (func 4))
  (export "while" (func 5))
  (export "for" (func 6)))
