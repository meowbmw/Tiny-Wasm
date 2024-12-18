#include "utils.h"
#include "json.hpp"
#include "Instuctions.hpp"

using namespace std;
using json = nlohmann::json;

int main() {
  // aarch64-linux-gnu-g++ -c arm64.s && aarch64-linux-gnu-objdump -d arm64.o
  // aarch64-linux-gnu-g++ parser.cpp -o parser && qemu-aarch64 -L /usr/aarch64-linux-gnu ./parser
//   string wasm_to_read = "learning_material/add.wasm";
  string wasm_to_read = "test/local.1.wasm";
  string s = readBinary(wasm_to_read);
  bool enable_export_decode = false;
  bool enable_code_decode = true;
  cout << "Full Binary: " << s << endl;
  const string magic_number = s.substr(0, 8);
  cout << "Initial checking..\nMagic number: " << ((magic_number == "0061736d") ? "Matched" : "Unmatched") << endl;
  s = s.substr(8); // crop magic number
  cout << "Webassembly version is: " << s.substr(0, 2) << endl;
  s = s.substr(8); // crop version
  while (s.size() > 0) {
    const string type = s.substr(0, 2);
    s = s.substr(2);                                                // crop type
    const unsigned int length = stoul(s.substr(0, 2), nullptr, 16); // Warn: we assume that the length is at max 1 byte!!!
    s = s.substr(2);                                                // crop length
    if (type == "07" && enable_export_decode) {
      // export section
      cout << "Decoding export section: " << s.substr(0, length * 2) << endl;
      const u_int64_t export_count = stoul(s.substr(0, 2), nullptr, 16);
      cout << "Total export count: " << export_count << endl;
      uint64_t base_offset = 0;
      for (int i = 0; i < export_count; ++i) {
        const u_int64_t export_size = stoul(s.substr(base_offset + 2, 2), nullptr, 16);
        cout << "--- Info for export " << i << " ---" << endl;
        const string export_name = hexToAscii(s.substr(base_offset + 4, export_size * 2));
        const u_int64_t export_type = stoul(s.substr(base_offset + 2 * export_size + 4, 2), nullptr, 16);
        const u_int64_t export_index = stoul(s.substr(base_offset + 2 * export_size + 6, 2), nullptr, 16);
        cout << "Type: " << export_type << endl;
        cout << "Index: " << export_index << endl;
        cout << "Name: " << export_name << endl;
        base_offset = base_offset + 2 * export_size + 6;
      }
    }
    if (type == "0a" && enable_code_decode) {
      // code section
      cout << "Decoding code section: " << s.substr(0, length * 2) << endl;
      const u_int64_t func_count = stoul(s.substr(0, 2), nullptr, 16);
      cout << "Total function count: " << func_count << endl;
      uint64_t base_offset = 0;
      for (int i = 0; i < func_count; ++i) {
        const u_int64_t func_size = stoul(s.substr(base_offset + 2, 2), nullptr, 16);
        const u_int64_t local_var_declare_count = stoul(s.substr(base_offset + 4, 2), nullptr, 16); //Warn!!! One declare could imply multiple variables so this does not really equal to the real variable count!!!
        cout << "--- Info for function " << i << " ---" << endl;
        cout << "Func size: " << func_size << endl;
        cout << "Local variable declare count: " << local_var_declare_count << endl;
        vector<string> opcodes;
        for (int i = 0; i < func_size - 1; ++i) {
          opcodes.push_back(s.substr(base_offset + 6 + i * 2, 2));
        }
        cout << "Opcode: ";
        for (auto &c : opcodes) {
          cout << c << " ";
        }
        cout << endl;
        base_offset = base_offset + 2 * func_size + 2;
        Instructions instr(opcodes, local_var_declare_count);
        instr.processOpcodes();
        // Instructions::processOpcodes(opcodes);
      }
    }
    s = s.substr(length * 2); // move forward, remeber we need to times 2 because we are processing 2 char at a time; 2 char = 1 byte
    // cout << type << " " << length << endl;
  }
}