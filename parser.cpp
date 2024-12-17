#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <sys/mman.h>

using namespace std;
string readBinary(string wasm_source = "add.wasm")
{
    ifstream file(wasm_source, ios::binary);
    stringstream ss;
    char byte;
    while (file.get(byte))
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(byte));
    }
    file.close();
    return ss.str();
}
class Section
{
public:
    int length;
    int type;
    string descriptor;
};
class Instructions
{
public:
    static void processOpcodes(vector<string> &vec)
    {
        for (auto &c : vec)
        {
            if (c == "0f")
            { // ret
                emitRet();
            }
            else if (c == "0b")
            { // end
                emitRet();
            }
        }
    }
    static void emitRet()
    {
        // cout << "Emitting Return" << endl;
        string instr = "d65f03c0";
        excuteInst(instr);
        // cout << "Executed" << endl;
    }
    static void printHexArray(unsigned char *charArray, int arraySize)
    {
        for (size_t i = 0; i < arraySize; i++)
        {
            std::cout << std::hex << setw(2) << setfill('0') << static_cast<unsigned int>(charArray[i]) << " ";
        }
        cout << endl;
    }
    static void excuteInst(string s)
    {
        int arraySize = s.length() / 2;
        unsigned char *charArray = (unsigned char *)malloc(arraySize);
        for (size_t i = 0; i < arraySize; i++)
        {
            string byteStr = s.substr(arraySize * 2 - i * 2 - 2, 2); // little endian on arm so we need to do this reversely!! no need to do this on x86
            charArray[i] = static_cast<unsigned char>(stoul(byteStr, nullptr, 16));
        }
        void (*func)() = nullptr;
        // allocate executable buffer
        func = reinterpret_cast<void (*)()>(
            mmap(0, arraySize, PROT_READ | PROT_WRITE | PROT_EXEC,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (func == MAP_FAILED)
        {
            perror("mmap");
            free(charArray);
            return;
        }
        // copy code to buffer
        memcpy(reinterpret_cast<void *>(func), charArray, arraySize);
        // ensure memcpy isn't optimized away as a dead store.
        __builtin___clear_cache(reinterpret_cast<char *>(func),
                                reinterpret_cast<char *>(func) + arraySize);
        func();
        munmap(reinterpret_cast<void *>(func), arraySize);
        free(charArray); 
    }
};
int main()
{
    // aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
    // aarch64-linux-gnu-g++ parser.cpp -o parser && qemu-aarch64 -L /usr/aarch64-linux-gnu ./parser
    string s = readBinary();
    cout << "Full Binary: " << s << endl;
    string magic_number = s.substr(0, 8);
    cout << "Initial checking..\nMagic number: " << ((magic_number == "0061736d") ? "Matched" : "Unmatched") << endl;
    s = s.substr(8); // crop magic number
    cout << "Webassembly version is: " << s.substr(0, 2) << endl;
    s = s.substr(8); // crop version
    int i = 0;
    while (s.size())
    {
        string type = s.substr(0, 2);
        s = s.substr(2);                                          // crop type
        unsigned int length = stoul(s.substr(0, 2), nullptr, 16); // Todo: we assume that the length is at max 1 byte!!!
        s = s.substr(2);                                          // crop length
        if (type == "0a")
        {
            cout << "Decoding code section: " << s.substr(0, length * 2) << endl;
            int func_count = stoul(s.substr(0, 2), nullptr, 16);
            int func_size = stoul(s.substr(2, 2), nullptr, 16);
            int local_var_count = stoul(s.substr(4, 2), nullptr, 16);
            cout << "Func count: " << func_count << endl;
            cout << "Func size: " << func_size << endl;
            cout << "Local var count: " << local_var_count << endl;
            vector<string> opcodes;
            for (int i = 0; i < func_size - 1; ++i)
            {
                opcodes.push_back(s.substr(6 + i * 2, 2));
            }
            cout << "Opcode: ";
            for (auto &c : opcodes)
            {
                cout << c << " ";
            }
            cout << endl;
            Instructions::processOpcodes(opcodes);
        }
        s = s.substr(length * 2); // move forward, remeber we need to times 2 because we are processing 2 char at a time; 2 char = 1 byte
        // cout << type << " " << length << endl;
    }
}