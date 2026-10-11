$ErrorActionPreference = 'Stop'
# CPU arithmetic/layout test only, not a Vita sampler or performance test.
$renderer = Get-Content (Join-Path $PSScriptRoot '../src/platform/vita/neptune_renderer.cpp') -Raw
$words = @([regex]::Matches($renderer, 'offsets\[[0-3]\] = 0x([0-9A-F]+)u;') | ForEach-Object {
    [Convert]::ToUInt32($_.Groups[1].Value, 16)
})
if ($words.Count -ne 4) { throw 'Expected four packed FP16 offset texels' }
function DecodeHalf([int]$bits) {
    if (($bits -band 0x7fff) -eq 0) { return 0.0 }
    $sign = if ($bits -band 0x8000) { -1.0 } else { 1.0 }
    $exponent = (($bits -shr 10) -band 31) - 15
    return $sign * (1.0 + ($bits -band 1023) / 1024.0) * [Math]::Pow(2.0, $exponent)
}
$expected = @(@(-0.25,-0.5), @(0.0,0.25), @(0.25,0.0), @(-0.5,-0.25))
for ($i = 0; $i -lt 4; ++$i) {
    $r = DecodeHalf ([int]($words[$i] -band 0xffff))
    $g = DecodeHalf ([int](($words[$i] -shr 16) -band 0xffff))
    if ($r -ne $expected[$i][0] -or $g -ne $expected[$i][1]) { throw "FP16 mismatch at Morton texel $i" }
}
# POINT + REPEAT uses floor(frac(WPOS / 2) * 2). Exercise both integer
# boundaries and half-pixel centers over the complete 960x544 viewport axes.
foreach ($extent in @(960,544)) {
    for ($p = 0; $p -lt $extent; ++$p) {
        foreach ($center in @(0.0,0.5)) {
            $coordinate = ($p + $center) * 0.5
            $lookup = [Math]::Floor(($coordinate - [Math]::Floor($coordinate)) * 2.0)
            if ($lookup -ne ($p % 2)) { throw "Screen parity mismatch at $p+$center" }
        }
    }
}
$cases = 0
$rng = [Random]::new(42)
foreach ($size in @(1,8,64,256,448,544,704,960,1024)) {
    foreach ($offset in @(-0.5,-0.25,0.0,0.25)) {
        for ($i = 0; $i -lt 2000; ++$i) {
            $uv = [Math]::Max(0.0, [Math]::Min(1.0, $rng.NextDouble() * 1.4 - 0.2))
            $reference = [Math]::Max(0, [Math]::Min($size-1, [Math]::Floor($uv*$size+$offset)))
            $sample = [Math]::Max(0.5/$size, [Math]::Min(1.0-0.5/$size, $uv+$offset/$size))
            if ([Math]::Floor($sample*$size) -ne $reference) { throw "Texel mismatch size=$size" }
            ++$cases
        }
    }
}
Write-Output "PASS: packed FP16 layout, full viewport axis parity, $cases randomized texel selections (CPU model only)"
