from pathlib import Path
import struct,sys
ram=Path(sys.argv[1]).read_bytes()
def v(ea,n):return int.from_bytes(ram[ea-0x6000000:ea-0x6000000+n], 'big')
def string(ea):return ram[ea-0x6000000:].split(b'\0',1)[0]
assert v(0x605B2B6,2)==0xE509
assert v(0x605B3B0,4)==0x60AF9F4
assert string(0x60AF9F4)==b'B_A5_03.PCM'
assert v(0x605B3B4,4)==0x6070E0E
assert v(0x605B2CE,2)==0xE34B
text=v(0x60AC9F8+4*0x20+12,4)
assert string(v(text+9*4,4))==b'Watch out for the ones with horns.'
assert string(v(text+4*4,4))==b'Edge, the red one is the queen.'
print('PASS original second opening: subtitle 9, B_A5_03.PCM, shared call, 75-frame handoff; first subtitle retained')
