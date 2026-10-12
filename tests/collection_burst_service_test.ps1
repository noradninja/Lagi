param([string]$ScratchRoot=(Join-Path $env:TEMP 'lagi-collection-burst'))
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$null=New-Item -ItemType Directory -Force -Path $ScratchRoot
$ScratchRoot=(Resolve-Path $ScratchRoot).Path
$source=(Get-Content -Raw (Join-Path $root 'src/integration/lagi_vdp1_animated_quad_vita.cpp')).Replace("`r`n","`n")
$begin=$source.IndexOf('int drawImmediateBillboardSprite(')
$end=$source.IndexOf('void flushParticleBillboards()', $begin)
if ($begin -lt 0 -or $end -le $begin) {throw 'Production collection service anchors missing'}
# Mechanically extract the actual service and its seven-argument native adapter.
Set-Content -LiteralPath (Join-Path $ScratchRoot 'collection_burst_body.inc') -Value $source.Substring($begin,$end-$begin) -Encoding utf8
Push-Location $ScratchRoot
try {
 & cl /nologo /std:c++20 /EHsc "/I$ScratchRoot" "/I$root/include" (Join-Path $PSScriptRoot 'collection_burst_service_test.cpp') /Fe:collection_burst_service_test.exe /Fo:collection_burst_service_test.obj
 if ($LASTEXITCODE -ne 0) {throw 'Collection service host fixture compile failed'}
 & .\collection_burst_service_test.exe
 if ($LASTEXITCODE -ne 0) {throw 'Actual collection service test failed'}
} finally {Pop-Location}
