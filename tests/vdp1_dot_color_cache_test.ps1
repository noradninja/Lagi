$ErrorActionPreference = 'Stop'
# Differential reference for the local dot-color memoization, not a GPU test.
# Includes zero results, every mask word/bit, repeated dots and a new palette.
foreach ($generation in @(0, 1)) {
    foreach ($mask in @(15, 63, 127, 255)) {
        $colors = New-Object 'UInt32[]' 256
        $valid = New-Object 'UInt32[]' 8
        $resolutions = 0
        for ($pass = 0; $pass -lt 3; ++$pass) {
            for ($raw = 0; $raw -lt 256; ++$raw) {
                $dot = $raw -band $mask
                $word = $dot -shr 5
                $bit = [uint32]([long]1 -shl ($dot -band 31))
                $expected = [uint32](($dot * 37 + $generation * 11) -band 65535)
                if (($valid[$word] -band $bit) -eq 0) {
                    $colors[$dot] = $expected
                    $valid[$word] = $valid[$word] -bor $bit
                    ++$resolutions
                }
                if ($colors[$dot] -ne $expected) { throw 'Color mismatch' }
            }
        }
        if ($resolutions -ne ($mask + 1)) { throw 'Repeated or skipped resolution' }
    }
}
$source = Get-Content -Raw "$PSScriptRoot/../src/platform/vita/neptune_renderer.cpp"
$start = $source.LastIndexOf('static bool decodeLiveVdp1Texture(')
$end = $source.IndexOf('static std::uint16_t liveTownTextureIndex(', $start)
$decode = $source.Substring($start, $end - $start)
if (-not $decode.Contains('std::uint32_t dotColorValid[8]{};') -or
    -not $decode.Contains('out.rgba[pixel] = cachedDotColor(dot, resolveColor);') -or
    -not $decode.Contains('out.rgba[p] = cachedDotColor(dot,') -or
    -not $decode.Contains('dotColorValid[word] |= bit;')) {
    throw 'Decoder cache integration changed; review reference coverage'
}
Write-Output 'PASS: dot-color cache masks, zero colors, repeated dots and per-decode reset reference'
