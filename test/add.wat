(module
  (func $add (export "add") (param $a i32) (param $b i32) (result i32)
    local.get 0
    local.get 1
    i32.add
  )
)