from capstone import *

code = bytes.fromhex("9B057DEA")
# md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
md = Cs(CS_ARCH_ARM64, CS_MODE_BIG_ENDIAN)
for i in md.disasm(code, 0x1000):
    print(i.mnemonic, i.op_str)
