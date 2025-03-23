import { readFileSync } from 'node:fs';
// Use the readFileSync function to read the contents of the "add.wasm" file
const wasmBuffer = readFileSync('/home/meow/Tiny-Wasm/test/CH13/main.wasm');
// Use the WebAssembly.instantiate method to instantiate the WebAssembly module
WebAssembly.instantiate(wasmBuffer, { env: {myPrintf: (num) => console.log(`WASM says: ${num}`)} }).then(wasmModule => {
    // Exported function lives under instance.exports object
    const { _start } = wasmModule.instance.exports;
    console.log(_start());
});