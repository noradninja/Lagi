$ErrorActionPreference = 'Stop'
# Compare the old whole-buffer clear + row copies with the new packed copy /
# padding-only clear. This tests upload byte layout, not GXM timing or caching.
$cases = 0
foreach ($width in ((1..32) + @(63,64,65,255,256,504))) {
    foreach ($height in @(1,2,17)) {
        $stride = ($width + 7) -band -8
        $pixels = [byte[]]::new($width * $height * 4)
        for ($i = 0; $i -lt $pixels.Length; ++$i) {
            $pixels[$i] = ($i * 17 + $width) -band 255
        }
        $reference = [byte[]]::new($stride * $height * 4)
        $actual = [byte[]]::new($reference.Length)
        for ($i = 0; $i -lt $actual.Length; ++$i) { $actual[$i] = 165 }
        for ($y = 0; $y -lt $height; ++$y) {
            [Buffer]::BlockCopy($pixels, $y*$width*4, $reference, $y*$stride*4, $width*4)
        }
        if ($stride -eq $width) {
            [Buffer]::BlockCopy($pixels, 0, $actual, 0, $actual.Length)
        } else {
            for ($y = 0; $y -lt $height; ++$y) {
                [Buffer]::BlockCopy($pixels, $y*$width*4, $actual, $y*$stride*4, $width*4)
                [Array]::Clear($actual, ($y*$stride+$width)*4, ($stride-$width)*4)
            }
        }
        for ($i = 0; $i -lt $actual.Length; ++$i) {
            if ($actual[$i] -ne $reference[$i]) {
                throw "Upload layout differs: ${width}x${height}, byte $i"
            }
        }
        ++$cases
    }
}
Write-Output "PASS: $cases texture layouts match, including dirty padding"
