Run command:
aarch64-linux-gnu-g++ parser.cpp -o parser && qemu-aarch64 -L /usr/aarch64-linux-gnu ./parser

Example output:
Full Binary: 0061736d010000000104016000000302010007070103666f6f00000a050103000f0b
Initial checking..
Magic number: Matched
Webassembly version is: 01
Decoding code section: 0103000f0b
Func count: 1
Func size: 3
Local var count: 0
Opcode: 0f 0b 