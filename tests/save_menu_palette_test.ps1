$ErrorActionPreference = 'Stop'
# Exhaustive palette-address model plus production source guard. This is not
# a visual menu test, save-file test or proof of complete load functionality.
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/platform/vita/neptune_renderer.cpp')
$start = $source.IndexOf('static std::uint32_t decodeSaveMenuNbgPixel(')
$end = $source.IndexOf('static void overlayAzelTextPlane', $start)
$decoder = $source.Substring($start,$end-$start)
if ($decoder -notmatch '5u \* 0x100u \+ \(paladdr \| colorIndex\)') { throw 'Save palette bank must be added, not ORed' }
$cases=0
$regressions=0
foreach ($bank in 0..7) {
    foreach ($dot in 1..255) {
        $pattern = $bank -shl 12
        $paladdr = ($pattern -band 0x7000) -shr 4
        $expected = 0x500 + (($bank -shl 8) -bor $dot)
        $actual = 5 * 0x100 + ($paladdr -bor $dot)
        $old = 0x500 -bor ($paladdr -bor $dot)
        if ($actual -ne $expected) { throw "Palette mismatch bank=$bank dot=$dot" }
        if ($old -ne $expected) { ++$regressions }
        ++$cases
    }
}
if (!$regressions) { throw 'Fixture must exercise the old OR bug' }
Write-Output "PASS: $cases native save-menu palette addresses, $regressions old-OR regression cases (CPU/source only)"

# Shared 16-color decoder: supplemental palette bits may overlap CAOS.
# Cover all hardware palette offset banks, pattern palette fields, SCN palette
# fields and nontransparent nibbles. Preserve the native out-of-range result;
# these checks validate address arithmetic, not CRAM wrapping or GPU output.
$start = $source.LastIndexOf('static std::uint32_t decodeVdp2Chsz1Pixel(')
$end = $source.IndexOf('static std::uint32_t decodeVdp2Nbg1Pixel(', $start)
$decoder = $source.Substring($start,$end-$start)
if ($decoder -notmatch 'caos \* 0x100u \+\s*\(paladdr \| colorIndex\)') { throw 'Shared palette bank must be added, not ORed' }
$cases=0
$regressions=0
foreach ($caos in 0..7) {
    foreach ($patternBank in 0..15) {
        foreach ($supplementalBank in 0..7) {
            foreach ($dot in 1..15) {
                $pattern = $patternBank -shl 12
                $scn = $supplementalBank -shl 5
                $paladdr = (($pattern -band 0xF000) -shr 8) -bor (($scn -band 0xE0) -shl 3)
                $expected = $caos * 256 + (($supplementalBank * 256) -bor ($patternBank * 16) -bor $dot)
                $actual = $caos * 0x100 + ($paladdr -bor $dot)
                $old = ($caos * 0x100) -bor ($paladdr -bor $dot)
                if ($actual -ne $expected) { throw "Shared palette mismatch CAOS=$caos pattern=$patternBank SCN=$scn dot=$dot" }
                if ($old -ne $expected) { ++$regressions }
                ++$cases
            }
        }
    }
}
if (!$regressions) { throw 'Shared fixture must exercise overlapping CAOS and SCN banks' }
Write-Output "PASS: $cases native 16-color palette addresses, $regressions old-OR regression cases (CPU/source only)"
