.PHONY: qemu-gdb
qemu-gdb: 
	/usr/bin/clang++ --target=aarch64-linux-gnu -std=c++20 -g parser.cpp -o parser && qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu parser