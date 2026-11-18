$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
# Fixture has a relative production-include path and runs from its own location.
$source = Join-Path $PSScriptRoot "battle_a5_scatter_sequence_test.cpp"
$outDir = Join-Path $env:TEMP "lagi-a5-scatter-fixture"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$exe = Join-Path $outDir "battle_a5_scatter_sequence_test.exe"
& cl /nologo /EHsc /std:c++20 $source "/Fe:$exe" "/Fo:$(Join-Path $outDir 'battle_a5_scatter_sequence_test.obj')"
if ($LASTEXITCODE -ne 0) { throw "A5 Scatter sequence compilation failed" }
& $exe
if ($LASTEXITCODE -ne 0) { throw "A5 Scatter sequence assertions failed" }
Write-Host "A5 Scatter sequence fixture passed"
