#include <functional>

#include "../include/WasmFunction.hpp"
int main() {
  WasmFunction().emitSetJmp();
  cout << endl;
  WasmFunction().emitLongJmp();
}