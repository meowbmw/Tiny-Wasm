.PHONY: qemu-gdb
qemu-gdb: 
	ccache /usr/bin/clang++ --target=aarch64-linux-gnu -std=c++20 -g main.cpp -o main && qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu main; rm main