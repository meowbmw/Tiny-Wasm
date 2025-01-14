.PHONY: build qemu-gdb clean
build:
	ccache /usr/bin/clang++ --target=aarch64-linux-gnu -std=c++20 -g ${curFile}.cpp -o ${curFile} -lcapstone
qemu-gdb: 
	qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu ${curFile}
clean:
	rm ${curFile}