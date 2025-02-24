(module
  (type (;0;) (func))
  (type (;1;) (func (result i32)))
  (type (;2;) (func (result i64)))
  (type (;3;) (func (param i32) (result i32)))
  (func (;0;) (type 0)
    block  ;; label = @1
      i32.const 0
      i32.const 1
      br_if 0 (;@1;)
      i32.ctz
      drop
    end)
  (func (;1;) (type 0)
    block  ;; label = @1
      i64.const 0
      i32.const 1
      br_if 0 (;@1;)
      i64.ctz
      drop
    end)
  (func (;2;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 1
      i32.const 1
      br_if 0 (;@1;)
      i32.ctz
    end)
  (func (;3;) (type 2) (result i64)
    block (result i64)  ;; label = @1
      i64.const 2
      i32.const 1
      br_if 0 (;@1;)
      i64.ctz
    end)
  (func (;4;) (type 3) (param i32) (result i32)
    block  ;; label = @1
      local.get 0
      br_if 0 (;@1;)
      i32.const 2
      return
    end
    i32.const 3)
  (func (;5;) (type 3) (param i32) (result i32)
    block (result i32)  ;; label = @1
      i32.const 10
      local.get 0
      br_if 0 (;@1;)
      drop
      i32.const 11
      return
    end)
  (func (;6;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 1
      i32.const 1
      br_if 0 (;@1;)
      i32.const 10
      i32.add
    end)
  (func (;7;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 10
      i32.const 1
      i32.const 1
      br_if 0 (;@1;)
      i32.sub
    end)
  (func (;8;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 0
      i32.const 1
      br_if 0 (;@1;)
      i32.eqz
    end)
  (func (;9;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 1
      i32.const 1
      br_if 0 (;@1;)
      i32.const 10
      i32.le_u
    end)
  (func (;10;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 8
      br 0 (;@1;)
      i32.const 1
      br_if 0 (;@1;)
      drop
      i32.const 7
    end)
  (func (;11;) (type 1) (result i32)
    block (result i32)  ;; label = @1
      i32.const 6
      i32.const 9
      br 0 (;@1;)
      br_if 0 (;@1;)
      drop
      i32.const 7
    end)
  (func (;12;) (type 1) (result i32)
    i32.const 1
    block (result i32)  ;; label = @1
      i32.const 2
      drop
      block (result i32)  ;; label = @2
        i32.const 4
        drop
        i32.const 8
        br 1 (;@1;)
        br 0 (;@2;)
      end
      drop
      i32.const 16
    end
    i32.add)
  (func (;13;) (type 1) (result i32)
    i32.const 1
    block (result i32)  ;; label = @1
      i32.const 2
      drop
      block (result i32)  ;; label = @2
        i32.const 4
        drop
        i32.const 8
        br 1 (;@1;)
        i32.const 1
        br_if 0 (;@2;)
        drop
        i32.const 32
      end
      drop
      i32.const 16
    end
    i32.add)
  (export "type-i32" (func 0))
  (export "type-i64" (func 1))
  (export "type-i32-value" (func 2))
  (export "type-i64-value" (func 3))
  (export "as-block-first" (func 4))
  (export "as-block-first-value" (func 5))
  (export "as-binary-left" (func 6))
  (export "as-binary-right" (func 7))
  (export "as-test-operand" (func 8))
  (export "as-compare-left" (func 9))
  (export "as-br_if-value" (func 10))
  (export "as-br_if-value-cond" (func 11))
  (export "nested-br-value" (func 12))
  (export "nested-br_if-value" (func 13)))
