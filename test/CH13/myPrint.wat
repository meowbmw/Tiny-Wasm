(module $myPrint.wasm
  (type (;0;) (func (param i32 i32) (result i32)))
  (type (;1;) (func))
  (import "env" "myPrintf" (func $myPrintf (type 0)))
  (func $_start (type 1)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    local.set 0
    i32.const 224
    local.set 1
    local.get 0
    local.get 1
    i32.sub
    local.set 2
    local.get 2
    global.set $__stack_pointer
    i32.const 20
    local.set 3
    local.get 2
    local.get 3
    i32.store
    i32.const 1203
    local.set 4
    local.get 4
    local.get 2
    call $myPrintf
    drop
    i32.const -20
    local.set 5
    local.get 2
    local.get 5
    i32.store offset=16
    i32.const 1165
    local.set 6
    i32.const 16
    local.set 7
    local.get 2
    local.get 7
    i32.add
    local.set 8
    local.get 6
    local.get 8
    call $myPrintf
    drop
    i32.const 20
    local.set 9
    local.get 2
    local.get 9
    i32.store offset=32
    i32.const 1067
    local.set 10
    i32.const 32
    local.set 11
    local.get 2
    local.get 11
    i32.add
    local.set 12
    local.get 10
    local.get 12
    call $myPrintf
    drop
    i32.const 88
    local.set 13
    local.get 2
    local.get 13
    i32.store offset=48
    i32.const 1050
    local.set 14
    i32.const 48
    local.set 15
    local.get 2
    local.get 15
    i32.add
    local.set 16
    local.get 14
    local.get 16
    call $myPrintf
    drop
    i32.const 99
    local.set 17
    local.get 2
    local.get 17
    i32.store offset=76
    i32.const 90
    local.set 18
    local.get 2
    local.get 18
    i32.store offset=72
    i32.const 89
    local.set 19
    local.get 2
    local.get 19
    i32.store offset=68
    i32.const 88
    local.set 20
    local.get 2
    local.get 20
    i32.store offset=64
    i32.const 1083
    local.set 21
    i32.const 64
    local.set 22
    local.get 2
    local.get 22
    i32.add
    local.set 23
    local.get 21
    local.get 23
    call $myPrintf
    drop
    i32.const 96
    local.set 24
    local.get 2
    local.get 24
    i32.add
    local.set 25
    i64.const 88
    local.set 26
    local.get 25
    local.get 26
    i64.store
    i32.const 76
    local.set 27
    local.get 2
    local.get 27
    i32.store offset=88
    i32.const 75
    local.set 28
    local.get 2
    local.get 28
    i32.store offset=84
    i32.const 74
    local.set 29
    local.get 2
    local.get 29
    i32.store offset=80
    i32.const 1028
    local.set 30
    i32.const 80
    local.set 31
    local.get 2
    local.get 31
    i32.add
    local.set 32
    local.get 30
    local.get 32
    call $myPrintf
    drop
    i32.const -88
    local.set 33
    local.get 2
    local.get 33
    i32.store offset=124
    i32.const 90
    local.set 34
    local.get 2
    local.get 34
    i32.store offset=120
    i32.const 89
    local.set 35
    local.get 2
    local.get 35
    i32.store offset=116
    i32.const 88
    local.set 36
    local.get 2
    local.get 36
    i32.store offset=112
    i32.const 1220
    local.set 37
    i32.const 112
    local.set 38
    local.get 2
    local.get 38
    i32.add
    local.set 39
    local.get 37
    local.get 39
    call $myPrintf
    drop
    i32.const 144
    local.set 40
    local.get 2
    local.get 40
    i32.add
    local.set 41
    i64.const -1024
    local.set 42
    local.get 41
    local.get 42
    i64.store
    i32.const 90
    local.set 43
    local.get 2
    local.get 43
    i32.store offset=136
    i32.const 89
    local.set 44
    local.get 2
    local.get 44
    i32.store offset=132
    i32.const 88
    local.set 45
    local.get 2
    local.get 45
    i32.store offset=128
    i32.const 1181
    local.set 46
    i32.const 128
    local.set 47
    local.get 2
    local.get 47
    i32.add
    local.set 48
    local.get 46
    local.get 48
    call $myPrintf
    drop
    i32.const 1024
    local.set 49
    local.get 2
    local.get 49
    i32.store offset=160
    i32.const 1128
    local.set 50
    i32.const 160
    local.set 51
    local.get 2
    local.get 51
    i32.add
    local.set 52
    local.get 50
    local.get 52
    call $myPrintf
    drop
    i32.const 1024
    local.set 53
    local.get 2
    local.get 53
    i32.store offset=180
    i32.const 82
    local.set 54
    local.get 2
    local.get 54
    i32.store offset=176
    i32.const 1103
    local.set 55
    i32.const 176
    local.set 56
    local.get 2
    local.get 56
    i32.add
    local.set 57
    local.get 55
    local.get 57
    call $myPrintf
    drop
    i32.const 65535
    local.set 58
    local.get 2
    local.get 58
    i32.store offset=220
    i32.const 220
    local.set 59
    local.get 2
    local.get 59
    i32.add
    local.set 60
    local.get 60
    local.set 61
    local.get 2
    local.get 61
    i32.store offset=216
    local.get 2
    i32.load offset=216
    local.set 62
    local.get 2
    local.get 62
    i32.store offset=192
    i32.const 1146
    local.set 63
    i32.const 192
    local.set 64
    local.get 2
    local.get 64
    i32.add
    local.set 65
    local.get 63
    local.get 65
    call $myPrintf
    drop
    local.get 2
    i32.load offset=216
    local.set 66
    i32.const 82
    local.set 67
    local.get 2
    local.get 67
    i32.store offset=212
    local.get 2
    local.get 66
    i32.store offset=208
    i32.const 1240
    local.set 68
    i32.const 208
    local.set 69
    local.get 2
    local.get 69
    i32.add
    local.set 70
    local.get 68
    local.get 70
    call $myPrintf
    drop
    i32.const 224
    local.set 71
    local.get 2
    local.get 71
    i32.add
    local.set 72
    local.get 72
    global.set $__stack_pointer
    return)
  (func $dummy (type 1))
  (func $__wasm_call_dtors (type 1)
    call $dummy
    call $dummy)
  (func $_start.command_export (type 1)
    call $_start
    call $__wasm_call_dtors)
  (table (;0;) 1 1 funcref)
  (memory (;0;) 1)
  (global $__stack_pointer (mut i32) (i32.const 9456))
  (export "memory" (memory 0))
  (export "__indirect_function_table" (table 0))
  (export "_start" (func $_start.command_export))
  (data $.rodata (i32.const 1024) "abc\00[test] %c %c %c %llu\0a\00[test] d is %lu\0a\00[test] d is %u\0a\00[test] %c %c %c %u\0a\00[test] c is %c, s is %s\0a\00[test] str is %s\0a\00[test] data is %p\0a\00[test] d is %i\0a\00[test] %c %c %c %lld\0a\00[test] -d is %d\0a\00[test] %c %c %c %d\0a\00[test] data is %p %c\0a\00"))
