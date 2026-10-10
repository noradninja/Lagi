$ErrorActionPreference = 'Stop'
# Verify UNORM reconstruction and both aligned half-word lanes, including
# invalid and signed payload bits. This is a CPU reference, not GPU validation.
for ($value = 0; $value -lt 65536; ++$value) {
    $lo = $value -band 255
    $hi = $value -shr 8
    $expected = $value -band 32767
    if ($expected -ge 16384) { $expected -= 32768 }
    foreach ($lane in @(0, 2)) {
        $channels = @(17, 39, 53, 71)
        $channels[$lane] = $lo
        $channels[$lane + 1] = $hi
        $decoded = @($channels | ForEach-Object {
            [Math]::Floor(([single]($_ / 255.0)) * 255.0 + 0.5)
        })
        $actual = $decoded[$lane] + ($decoded[$lane + 1] % 128) * 256
        if ($actual -ge 16384) { $actual -= 32768 }
        if ($actual -ne $expected -or
            (($decoded[$lane + 1] -ge 128) -ne ($value -ge 32768))) {
            throw "Half-word mismatch: value=$value lane=$lane"
        }
    }
}
foreach ($payload in @(0, 1, 65535, 65536, 8388607, 8388608, 16777215)) {
    foreach ($flag in @(0, 128, 255)) {
        $bytes = @(($payload -band 255), (($payload -shr 8) -band 255),
            (($payload -shr 16) -band 255), $flag)
        $actual = $bytes[0] + $bytes[1] * 256 + $bytes[2] * 65536
        if ($bytes[2] -ge 128) { $actual -= 16777216 }
        $expected = $payload
        if ($payload -ge 8388608) { $expected -= 16777216 }
        if ($actual -ne $expected) { throw 'Full-word sign mismatch' }
    }
}
Write-Output 'PASS: coefficient half-word lanes, UNORM bytes, invalid and signed payloads'
