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
Write-Output "PASS: $cases signed integer wrap cases match"
