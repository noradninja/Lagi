"""Check native Scatter instruction boundaries in an original BTL_A5 RAM capture.
Usage: python tests/nanyd_scatter_evidence_test.py <original-high-work-ram.bin>
"""
from pathlib import Path
import struct, sys
ram=Path(sys.argv[1]).read_bytes()
def value(ea,n):
    return int.from_bytes(ram[ea-0x6000000:ea-0x6000000+n], 'big')
for ea,expected,n in [
 (0x605CE7C,0x6061B9A,4), (0x605CE64,0xF0,2),
 (0x605CE42,0xE309,2), (0x605D12A,0xE0,2),
 (0x605D0DC,0x22BB,2), (0x605D0FA,0xE528,2),
 (0x605D0FE,0xE403,2), (0x605D140,0x10000,4),
 (0x605D120,0xE30B,2), (0x605F0F4,0x28000,4),
 (0x605F0FC,0xFFFEC000,4), (0x605F104,0x60000,4),
 (0x605E9AE,0xE25A,2), (0x605EB6C,0xFFF,2), (0x605EB6E,0xFF,2),
]:
    assert value(ea,n)==expected, f'Native contract differs at {ea:08X}'
assert 0x605CEA4+value(0x605CEA8+9*2,2)==0x605D0A0
print('PASS original Scatter: gates, dispatch, flags, escape modulo/scaling, springs and timer')
