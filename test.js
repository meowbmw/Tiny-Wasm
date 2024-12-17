import { readFileSync } from 'node:fs';
// Use the readFileSync function to read the contents of the "add.wasm" file
const wasmBuffer = readFileSync('add.wasm');
// Use the WebAssembly.instantiate method to instantiate the WebAssembly module
WebAssembly.instantiate(wasmBuffer).then(wasmModule => {
    // Exported function lives under instance.exports object
    const { foo } = wasmModule.instance.exports;
    foo();
});