# Tiny Wasm Compiler

## Prerequisite:
    OS: Ubuntu 24.04.1 x86-64
    
    dpkg --add-architecture arm64
    
    Edit /etc/apt/sources.list.d/ubuntu.sources with following:

    Types: deb
    URIs: http://archive.ubuntu.com/ubuntu/
    Suites: noble
    Components: main restricted universe
    Architectures: amd64
    Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
    Trusted: yes

    Types: deb
    URIs: http://security.ubuntu.com/ubuntu/
    Suites: noble-security
    Components: main restricted universe
    Architectures: amd64
    Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
    Trusted: yes

    Types: deb
    URIs: http://archive.ubuntu.com/ubuntu/
    Suites: noble-updates
    Components: main restricted universe
    Architectures: amd64
    Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
    Trusted: yes

    Types: deb
    URIs: http://azure.ports.ubuntu.com/ubuntu-ports/
    Suites: noble
    Components: main restricted multiverse universe
    Architectures: arm64
    Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
    Trusted: yes

    Types: deb
    URIs: http://azure.ports.ubuntu.com/ubuntu-ports/
    Suites: noble-updates
    Components: main restricted multiverse universe
    Architectures: arm64
    Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
    Trusted: yes

    sudo apt install libcapstone-dev:arm64 python3-capstone nlohmann-json3-dev libgtest-dev build-essential gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
## Run command:

    aarch64-linux-gnu-g++ main.cpp -o main && qemu-aarch64 -L /usr/aarch64-linux-gnu ./main

## Test command (With GoogleTest):
### (1) Compile gtest with cmake:

    cmake -S . -B build && cmake --build build

### (2) Run gtest:

    cd ./build && ctest && cd ..

## Example output:

    Parsing wasm file: test/local.2.wasm
    Full Binary: 0061736d010000000116056000017f6000017e60017f017f60017e017e600000030706000102030402076f060e747970652d6c6f63616c2d69333200000e747970652d6c6f63616c2d69363400010e747970652d706172616d2d69333200020e747970652d706172616d2d69363400031261732d6c6f63616c2e7365742d76616c756500041261732d6c6f63616c2e7465652d76616c756500050a35060801017f410122000b0801017e420122000b0600410a22000b0600420b22000b0a01017f4101220021000b08004101220022000b
    Initial checking..
    Magic number: Matched
    Webassembly version is: 01
    Decoding type section: 056000017f6000017e60017f017f60017e017e600000
    Total type count: 5
    Decoding function section: 06000102030402
    Total function count: 6
    Decoding export section: 060e747970652d6c6f63616c2d69333200000e747970652d6c6f63616c2d69363400010e747970652d706172616d2d69333200020e747970652d706172616d2d69363400031261732d6c6f63616c2e7365742d76616c756500041261732d6c6f63616c2e7465652d76616c75650005
    Total export count: 6
    Decoding code section: 060801017f410122000b0801017e420122000b0600410a22000b0600420b22000b0a01017f4101220021000b08004101220022000b
    Total function count: 6
    ------ Processing function 0: type-local-i32 ------
    --- Loading params to their respective registers ---
    No params need to be load
    ---Loading parameters finished---
    Origin WASM Opcode: 41 01 22 00 0b 
    --- Printing PARAM data---
    Empty! Nothing here
    --- Printing LOCAL data---
    LOCAL[0]: (i) = 0
    --- Estimate stack allocation ---
    Param start location: 0
    Param end location: 0
    Local start location: 8
    Local end location: 12
    Wasm stack start location: 20
    Adding maximum possible wasm stack size: (code_vec.size: 7 - offset: 2) * 4 = 20
    Wasm stack end location: 40
    Stack allocate size estimated to be: 48
    Sub sp register
    Emit: sub sp, sp, 48 | D100C3FF
    Emit: str wzr, [sp, #12] | B9000FFF
    --- Printing initial stack ---
    [sp, #12] = LOCAL[0]
    --- JITing wasm code ---
    *Current wasm stack pointer is: 40
    i32.const 1
    Emit: mov 1, w11 | 72A0000B5280002B
    Emit: str w11, [sp, #40] | B9002BEB
    *Current wasm stack pointer is: 32
    Local.tee 0
    Assigning to LOCAL[0]
    Emit: ldr w11, [sp, #40] | B9402BEB
    Emit: str w11, [sp, #12] | B9000FEB
    *Current wasm stack pointer is: 32
    Moving stack top to register as answer
    Emit: ldr w0, [sp, #40] | B9402BE0
    Restore sp register
    Emit: add sp, sp, 48 | 9100C3FF
    Emit: ret | D65F03C0
    *Current wasm stack pointer is: 32
    --- Printing stack---
    stack[0]: (i) = 1
    Total param count: 0
    Total local count: 1
    Total result count: 1
    Executing function 0: type-local-i32
    Machine instruction to load: 01000014FFC300D1FF0F00B92B0080520B00A072EB2B00B9EB2B40B9EB0F00B9E02B40B9FFC30091C0035FD6