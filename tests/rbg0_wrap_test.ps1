$ErrorActionPreference = 'Stop'
# Arithmetic equivalence test only; not a shader image or GPU timing test.
$cases = 0
foreach ($dimension in @(512,1024,2048,4096)) {
    foreach ($coordinate in ((-16384..16384) + @(-8388607,8388607,-16777216,16777216))) {
        $sample = [float]$coordinate
        $reference = [float]($sample % $dimension)
        if ($reference -lt 0) { $reference = [float]($reference + $dimension) }
        $scaled = [float]($sample / $dimension)
        $fraction = [float]($scaled - [Math]::Floor($scaled))
        $actual = [float]($fraction * $dimension)
        if ($actual -ne $reference) {
            throw "Wrap mismatch: coordinate=$coordinate dimension=$dimension"
        }
        ++$cases
    }
}
# PLSZ=0 gives each selected plane exactly one page. The shader's direct
# in-plane coordinates and packed slot must match the general page formula.
foreach ($coordinate in ((-16384..16384) + @(-8388607,8388607,-16777216,16777216))) {
    $inPlane = [float](([float]$coordinate / 512) - [Math]::Floor([float]$coordinate / 512)) * 512
    $page = [Math]::Floor($inPlane / 512)
    if ($page -ne 0 -or ($inPlane - $page * 512) -ne $inPlane) {
        throw "Single-page coordinate mismatch: coordinate=$coordinate"
    }
    foreach ($packed in @(0,1,2,3,4,15,63,255)) {
        if (($packed % 4) -ne ([Math]::Floor($packed / [Math]::Pow(4,$page)) % 4)) {
            throw "Single-page slot mismatch: packed=$packed"
        }
    }
}
Write-Output "PASS: $cases signed integer wrap cases match"
