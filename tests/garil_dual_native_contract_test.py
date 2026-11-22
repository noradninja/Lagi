from pathlib import Path
import sys
p=Path(sys.argv[1]).read_bytes();base=0x6054000
anchors={0x605F124:(0xC901,2),0x605F126:(0x8807,2),0x605F12A:(0x8808,2),0x605F14A:(0xE311,2),0x605F14C:(0xE20F,2),0x605F208:(0xEC,2),0x605F20A:(0x1DC,2),0x605F240:(0x605E220,4),0x605F390:(0x605BBD0,4),0x605F234:(0x60B11C8,4),0x605F238:(0x60B13A8,4),0x605F388:(0x60B13B4,4),0x605F384:(0xFFFED000,4),0x605F366:(0xE0,2),0x605F372:(0x1D0,2),0x605F370:(0x200,2),0x605F28A:(0x4C08,2),0x605F28E:(0x4C00,2),0x605F296:(0x4C08,2)}
for ea,(value,n) in anchors.items():
 assert int.from_bytes(p[ea-base:ea-base+n],'big')==value,f'Original dual constructor contract changed at {ea:08X}'
print('PASS original dual actor factories, group IDs, layout/trail tables, second-block position and native RNG decision; evidence only, not runtime restoration')
