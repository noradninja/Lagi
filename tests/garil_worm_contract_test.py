"""Read-only contracts from the original Disc 1 FLD_A5.PRG.
Usage: python tests/garil_worm_contract_test.py <FLD_A5.PRG>
"""
from pathlib import Path
import sys
p=Path(sys.argv[1]).read_bytes()
def value(ea,n=4):
 o=ea-0x6054000
 assert 0<=o<=len(p)-n
 return int.from_bytes(p[o:o+n], 'big')
for ea,expected,n in [
 (0x6066844,0x609D7BC,4),
 (0x609D7BC,0x6066028,4),(0x609D7C0,0x60662BC,4),(0x609D7C4,0x6066584,4),
 (0x609D7CC,0x60666F8,4), # A different adjacent actor, not this factory.
 (0x6067F64,0x609E790,4),(0x6067F70,0x609DBC6,4),
 (0x6067F6C,0xFFE22000,4),(0x6067F44,0x74B,2),
 (0x606FA24,0x60720C6,4),(0x6072194,0x609F348,4),
 (0x609F34C,0x6071C92,4),
 (0x6071D1A,0x50E9,2),(0x6071D22,0x6332,2),
 (0x6071D26,0x323C,2),(0x6071D28,0x1E25,2),
 (0x6071D30,0x323C,2),(0x6071D32,0x1E26,2),
 (0x6071D3A,0x323C,2),(0x6071D4C,0x1E27,2),
]:
 assert value(ea,n)==expected,f'Original worm/camera contract differs at {ea:08X}'
print('PASS original worm factory, actor identity, tracker threshold/flag, shared camera dispatch and live target addition')

# Memory area 1 is FLD_A5.MCB, not the subfield-only FLD_A5_0.MCB.
import struct
bundle=Path(sys.argv[1]).with_name('FLD_A5.MCB').read_bytes()
def ptr(o):
 assert 0<=o<=len(bundle)-4
 return int.from_bytes(bundle[o:o+4],'big')
def bones(o,seen):
 if not o:return 0
 assert o not in seen and 0<=o<=len(bundle)-12
 seen.add(o)
 return 1+bones(ptr(o+4),seen)+bones(ptr(o+8),seen)
for kind in range(3):
 for m,q,a in [(0x609D744,0x609D75C,0x609D774),(0x609D750,0x609D768,0x609D780)]:
  model,pose,animation=[value(ea+kind*4) for ea in (m,q,a)]
  count=bones(ptr(model),set());offset=ptr(animation)
  flags,animated,frames=struct.unpack_from('>HHH',bundle,offset)
  assert flags and frames and animated>=count
  assert ptr(pose)+animated*36<=len(bundle)
for ea,expected,n in [(0x6067E1E,0x5134,2),(0x6067DF0,0x7202,2),
 (0x6067E30,0x800,2),(0x6067E32,0x400,2),(0x6067E34,0x111,2),
 (0x6067E40,0x888888,4),(0x6067E54,0x9075,2)]:
 assert value(ea,n)==expected,f'Native tracker contract differs at {ea:08X}'
print('PASS shared-field bundle bindings for all worm types and native Z/control stepping contracts')
