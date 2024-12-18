#include <cstdio>
#include <cstring>
#include <string>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
using namespace std;
int main()
{
    unsigned char code[] = {static_cast<unsigned char>(0x8D), static_cast<unsigned char>(0x04), static_cast<unsigned char>(0x37), // lea eax,[rdi+rsi]
                            static_cast<unsigned char>(0xC3)};

    for (size_t i = 0; i < 4; i++)
    {
        std::cout << std::hex << setw(2) << setfill('0') << static_cast<unsigned int>(code[i]) << " ";
    }
    cout << endl;
    int (*sum)(int, int) = nullptr;

    // allocate executable buffer
    sum = reinterpret_cast<int (*)(int, int)>(
        mmap(0, sizeof(code), PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    // copy code to buffer
    memcpy(reinterpret_cast<void *>(sum), code, sizeof(code));
    // ensure memcpy isn't optimized away as a dead store.
    __builtin___clear_cache(reinterpret_cast<char *>(sum),
                            reinterpret_cast<char *>(sum) + sizeof(code));

    // run code
    int a = 3;
    int b = 3;
    int c = sum(a, b);

    printf("%d + %d = %d\n", a, b, c);

    // free the allocated memory
    munmap(reinterpret_cast<void *>(sum), sizeof(code));

    return 0;
}