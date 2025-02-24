(module
  (func (export "singular") (param i32) (result i32)
    (if (local.get 0) (then (nop)))
    (if (local.get 0) (then (nop)) (else (nop)))
    (if (result i32) (local.get 0) (then (i32.const 7)) (else (i32.const 8)))
  )

  (func (export "as-return-value") (param i32) (result i32)
    (if (result i32) (local.get 0)
      (then (i32.const 1))
      (else (i32.const 0)))
    (return)
  )

  (func (export "as-local.set-value") (param i32) (result i32)
    (local i32)
    (local.set 0
      (if (result i32) (local.get 0)
        (then (i32.const 1))
        (else (i32.const 0))
      )
    )
    (local.get 0)
  )

  (func (export "as-local.tee-value") (param i32) (result i32)
    (local.tee 0
      (if (result i32) (local.get 0)
        (then (i32.const 1))
        (else (i32.const 0))
      )
    )
  )

  (func (export "abs") (param $x i32) (result i32)
    ;; Check if the number is negative
    (i32.lt_s (local.get $x) (i32.const 0))
    ;; If negative, return the negation of the number
    (if (result i32)
      (then
        (i32.sub (i32.const 0) (local.get $x))
      )
      ;; Else, return the number itself
      (else
        (local.get $x)
      )
    )
  )
)

(assert_return (invoke "abs" (i32.const 5)) (i32.const 5))
(assert_return (invoke "abs" (i32.const -5)) (i32.const 5))
(assert_return (invoke "singular" (i32.const 0)) (i32.const 8))
(assert_return (invoke "singular" (i32.const 1)) (i32.const 7))
(assert_return (invoke "singular" (i32.const 10)) (i32.const 7))
(assert_return (invoke "singular" (i32.const -10)) (i32.const 7))
(assert_return (invoke "as-return-value" (i32.const 0)) (i32.const 0))
(assert_return (invoke "as-return-value" (i32.const 1)) (i32.const 1))
(assert_return (invoke "as-local.set-value" (i32.const 0)) (i32.const 0))
(assert_return (invoke "as-local.set-value" (i32.const 1)) (i32.const 1))
(assert_return (invoke "as-local.tee-value" (i32.const 0)) (i32.const 0))
(assert_return (invoke "as-local.tee-value" (i32.const 1)) (i32.const 1))