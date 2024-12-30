.PHONY: qemu-gdb
qemu-gdb: 
	/usr/bin/aarch64-linux-gnu-g++ -std=c++20 -fdiagnostics-color=always -g parser.cpp -o parser && qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu parser