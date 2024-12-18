import io
import leb128

# unsigned leb128
print(leb128.u.encode(624485))
assert leb128.u.encode(624485) == bytearray([0xe5, 0x8e, 0x26])
assert leb128.u.decode(bytearray([0xe5, 0x8e, 0x26])) == 624485
assert leb128.u.decode_reader(io.BytesIO(bytearray([0xe5, 0x8e, 0x26]))) == (624485, 3)

#   signed leb128
assert leb128.i.encode(-12345) == bytearray([0xc7, 0x9f, 0x7f])
assert leb128.i.decode(bytearray([0xc7, 0x9f, 0x7f])) == -12345
assert leb128.i.decode_reader(io.BytesIO(bytearray([0xc7, 0x9f, 0x7f]))) == (-12345, 3)