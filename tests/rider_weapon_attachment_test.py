"""Pin rider attachment adaptation to original Disc 1 pointer-array instructions.
This is source/data parity validation, not a hardware rendering test.
"""
import argparse
from pathlib import Path
import re
import struct

parser = argparse.ArgumentParser()
parser.add_argument("--disc", type=Path, required=True)
parser.add_argument("--generated", type=Path, required=True)
args = parser.parse_args()

with args.disc.open("rb") as disc:
    def read_overlay(lba, base, address, size):
        offset = address - base
        result = bytearray()
        while len(result) < size:
            sector, within = divmod(offset, 2048)
            count = min(size - len(result), 2048 - within)
            disc.seek((lba + sector) * 2352 + 16 + within)
            chunk = disc.read(count)
            if len(chunk) != count:
                raise AssertionError("truncated disc image")
            result.extend(chunk)
            offset += count
        return bytes(result)

    def words(lba, address, expected):
        actual = read_overlay(lba, 0x06054000, address, len(expected) * 2)
        assert actual == struct.pack(">" + "H" * len(expected), *expected), hex(address)

    # Both paths read rider+5C = model+44, then a four-byte pointer slot.
    # Native battle: level8 -> pointer +24; other levels -> pointer +14.
    words(258144, 0x0609FE2E, [0xE05C, 0x045E, 0x5449])
    words(258144, 0x0609FE4C, [0xE05C, 0x045E, 0x5445])
    # Flight uses the same ordinary slot, independent of the battle adapter.
    words(244544, 0x0607449E, [0xE05C, 0x6492, 0x044E, 0x5445])
    # Edge COMMON hotpoint descriptor: only bone5 has the weapon point.
    descriptors = read_overlay(182, 0x00200000, 0x002020FC, 6 * 8)
    counts = [struct.unpack_from(">II", descriptors, bone * 8)[1]
              for bone in range(6)]
    assert counts == [0, 0, 0, 0, 0, 1], counts

source = args.generated.read_text()
start = source.index("static void s_battleDragon_DrawRiderWeapon(")
end = source.index("// BTL_A3::0609f258", start)
body = source[start:end]
slots = re.findall(r"m44_hotpointData\[(\d+)\]", body)
assert slots == ["9", "9", "5", "5"], slots
assert "mainGameState.gameStats.m1_dragonLevel == 8" in body
assert body.count("pThis->mF0 = pThis->m8_position;") == 2
assert "cameraProperties2.m28[0]" in body
assert "get3DModel(pRider1State->m14_weaponModelIndex)" in body
print("PASS: native battle/flight pointer slots, authored Edge hotpoint, generated battle attachment")
