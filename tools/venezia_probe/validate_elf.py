"""Build-time mirror of the conservative ARM loader check."""
import pathlib
import struct
import sys

def check(p):
    def req(ok, msg):
        if not ok:
            raise ValueError(msg)
    req(768 <= len(p) <= 256*1024 and p[:7] == b"\x7fELF\x01\x01\x01", "ELF32 little-endian/size")
    et, em, ev, entry, phoff, shoff, flags, eh, pe, pn, se, sn, _ = struct.unpack_from("<HHIIIIIHHHHHH", p, 16)
    req((et, em, ev, eh) == (2, 0xf00d, 1, 52), "not executable MeP ELF")
    def span(o,s):
        return o+s <= len(p) and not (s and o<768 and o+s>256)
    req(sn and se==40 and span(shoff,se*sn), "section table bounds/mailbox")
    req(not pn or (pe==32 and span(phoff,pe*pn)), "program table bounds/mailbox")
    found=False
    for i in range(sn):
        _, t, f, a, o, s, *_=struct.unpack_from("<10I", p, shoff+i*40)
        req(not (t==8 and s and f&2), "allocated BSS is unsupported")
        req(t==8 or span(o,s), "section overlaps mailbox or file bounds")
        found |= t==1 and f&6==6 and a==o and a<=entry<a+s
    req(found and entry>=768 and entry%2==0, "entry must be executable file offset")
    return entry

if __name__=="__main__":
    try:
        p=pathlib.Path(sys.argv[1]).read_bytes()
        print("Validated MeP ELF:", len(p), "bytes; entry", hex(check(p)))
    except (ValueError, OSError, struct.error) as e:
        sys.exit(str(e))
