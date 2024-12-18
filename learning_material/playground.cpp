#include <cstdio>
#include <cstring>
#include <string>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
using namespace std;
int main()
{
    // aarch64-linux-gnu-g++ playground.cpp -o playground && qemu-aarch64 -L /usr/aarch64-linux-gnu ./playground
    //  listen mode: aarch64-linux-gnu-g++ hello.cpp hello.s -o hello && qemu-aarch64 -g 1234 -L /usr/aarch64-linux-gnu ./hello
    string s = "d65f03c0";
    //          01234567
    int arraySize = s.length() / 2;
    unsigned char *charArray = (unsigned char *)malloc(arraySize);
    for (size_t i = 0; i < arraySize; i++)
    {
        string byteStr = s.substr(arraySize * 2 - i * 2 - 2, 2); // little endian on arm so we need to do this reversely!! no need to do this on x86
        charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
    }
    // for (size_t i = 0; i < arraySize; i++)
    // {
    //     std::cout << std::hex << setw(2) << setfill('0') << static_cast<unsigned int>(charArray[i]) << " ";
    // }
    // cout << endl;

    void (*hello)() = nullptr;

    // allocate executable buffer
    hello = reinterpret_cast<void (*)()>(
        mmap(0, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    if (hello == MAP_FAILED)
    {
        perror("mmap");
        free(charArray);
        return 1;
    }

    // copy code to buffer
    memcpy(reinterpret_cast<void *>(hello), charArray, arraySize);
    // ensure memcpy isn't optimized away as a dead store.
    __builtin___clear_cache(reinterpret_cast<char *>(hello),
                            reinterpret_cast<char *>(hello) + arraySize);

    hello();

    // free the allocated memory
    // munmap(reinterpret_cast<void *>(hello), arraySize);

    return 0;
}