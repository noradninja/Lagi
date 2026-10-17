param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference = 'Stop'
# US Disc 1 only. Read MODE1/2352 user sectors; never extract or alter data.
$stream = [IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
    $program = [byte[]]::new(253650)
    for ($at = 0; $at -lt $program.Length; $at += 2048) {
        [void]$stream.Seek((58 + [long]($at / 2048)) * 2352 + 16, [IO.SeekOrigin]::Begin)
        $remaining = [Math]::Min(2048, $program.Length - $at)
        $done = 0
        while ($done -lt $remaining) {
            $read = $stream.Read($program, $at + $done, $remaining - $done)
            if ($read -eq 0) { throw 'Truncated disc image' }
            $done += $read
        }
    }
} finally { $stream.Dispose() }
function Read-BE([uint32]$address, [int]$count) {
    $offset = [long]$address - 0x06006000
    if ($offset -lt 0 -or $offset + $count -gt $program.Length) { throw 'Address outside MAIN' }
    [uint64]$value = 0
    for ($i = 0; $i -lt $count; ++$i) { $value = ($value -shl 8) -bor $program[$offset + $i] }
    return $value
}
function Assert-Literal([uint32]$address, [uint64]$expected, [int]$count, [string]$meaning) {
    $actual = Read-BE $address $count
    if ($actual -ne $expected) {
        throw ('Wrong disc/version or changed evidence at {0:X8}: expected {1:X}, found {2:X}' -f $address,$expected,$actual)
    }
    '{0:X8}: {1:X8}  {2}' -f $address,$actual,$meaning
}
# Literal-pool entries, not instruction decoding. Addresses follow direct
# read-only disassembly of backup 0602849A and linked-list helpers.
Assert-Literal 0x0602849A 0x2FE6 2 'backup entry prologue'
Assert-Literal 0x06028534 0x0604FD00 4 'projection source'
Assert-Literal 0x06028538 0x0604B224 4 'VDP2 controls source'
Assert-Literal 0x06028638 0x0604B484 4 'optional fade source'
Assert-Literal 0x0602863C 0x0604B4D4 4 'additional VDP2 state source'
Assert-Literal 0x06028640 0x06052B2C 4 'light setup source'
Assert-Literal 0x06028644 0x25F00000 4 'CRAM source'
Assert-Literal 0x0602864C 0x0600FFC4 4 'resident list snapshot size helper'
Assert-Literal 0x06028650 0x0600FFDE 4 'resident list capture helper'
Assert-Literal 0x06028654 0x25E3E000 4 'VRAM source'
Assert-Literal 0x06028658 0x25E3C000 4 'optional VRAM source'
Assert-Literal 0x0602862A 0x120 2 'additional VDP2 state size'
Assert-Literal 0x0602862C 0x1000 2 'CRAM size'
Assert-Literal 0x0602862E 0x400 2 'VRAM span size'
Assert-Literal 0x06028630 0x800 2 'optional VRAM allocation size'
Assert-Literal 0x060101C8 0x0604A780 4 'resident linked-list head'
Assert-Literal 0x060101CC 0x25A0B000 4 'resident reload exclusion start'
Assert-Literal 0x060101D0 0x25A80000 4 'resident reload exclusion end'
Assert-Literal 0x060325B8 0x0604B4DC 4 '14-entry transfer pool base (state +8)'
Assert-Literal 0x060325BC 0x0604B4D4 4 'two transfer heads base'
Assert-Literal 0x06032576 0xE60E 2 'transfer initialization count 14'
Assert-Literal 0x06032580 0x7414 2 'transfer initialization stride 0x14'
Assert-Literal 0x06032592 0x7408 2 'transfer head initial target state +8'
'PASS: original handoff literals match; runtime and resource reload remain unverified.'
