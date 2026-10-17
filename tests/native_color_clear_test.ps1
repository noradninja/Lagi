param([string]$ScratchRoot='../native-color-clear-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$begin=$source.IndexOf('static bool clearNativeSceneColorBuffer(')
$end=$source.IndexOf('static void drawColorOffsetPass(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native scene clear boundary changed'}
[IO.File]::WriteAllText("$scratch/clear.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/native_color_clear_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native color clear compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Native color clear test failed'}
