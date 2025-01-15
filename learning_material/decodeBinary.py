from capstone import *

should_be = "FF8300D1FF0F00B9EB0F40B9EB1B00B9E01B40B9FF830091C0035FD6"
s = "135000A9155801A9176002A9196803A91B7004A91D7805A9E2030091023400F900008052C0035FD6"
code = bytes.fromhex(s)
md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
# md = Cs(CS_ARCH_ARM64, CS_MODE_BIG_ENDIAN)
for i in md.disasm(code, 0x1000):
    print(i.mnemonic, i.op_str)
