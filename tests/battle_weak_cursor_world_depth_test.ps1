$ErrorActionPreference = 'Stop'
$source = Join-Path $PSScriptRoot 'battle_weak_cursor_world_depth_test.cpp'
$dir = Join-Path $env:TEMP 'lagi-weak-depth-fixture'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$exe = Join-Path $dir 'weak_depth.exe'
& cl /nologo /EHsc /std:c++20 $source "/Fe:$exe" "/Fo:$(Join-Path $dir 'weak_depth.obj')"
if ($LASTEXITCODE -ne 0) { throw 'Weak-marker world-depth compilation failed' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Weak-marker world-depth assertions failed' }
Write-Host 'Weak-marker depth fixture passed'
