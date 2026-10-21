$ErrorActionPreference = 'Stop'
# Integer-pixel scale 176, position 1 world unit, depth 2 world units.
# Match the adapter's nested fixed-point multiplies.
$scale = 176L
$position = 65536L
$depth = 131072L
$inverseDepth = [long](65536L * 65536L / $depth)
$ratio = ($position * $inverseDepth) -shr 16
$pixels = ($scale * $ratio) -shr 16
if ($pixels -ne 88 -or ($pixels -shr 16) -ne 0) {
    throw 'Integer-pixel projection reference mismatch'
}
$source = Get-Content -Raw "$PSScriptRoot/../src/integration/lagi_vdp1_animated_quad_vita.cpp"
$start = $source.IndexOf('const fixedPoint invZ = FP_Div(0x10000, viewPos[2]);')
$end = $source.IndexOf('auto clamp16', $start)
if($start -lt 0 -or $end -le $start){throw 'Native X/Y projection boundary changed'}
$projection = $source.Substring($start, $end - $start)
# Renderer-side view-Z metadata legitimately converts 16.16 world units to
# float. Only screen X/Y, width and height must retain integer-pixel products.
if ($projection.Contains('.getInteger()') -or $projection.Contains('/ 65536.0f')) {
    throw 'Native sprite adapter reintroduced fixed-point pixel conversion'
}
Write-Output 'PASS: integer-pixel projection reference and adapter unit guard'
