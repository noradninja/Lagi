param([string]$ScratchRoot='../launcher-gxm-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$begin=$source.IndexOf('// GPU launcher presentation.')
$end=$source.IndexOf('// Capture the last presented launcher frame once',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'GPU launcher boundary changed'}
[IO.File]::WriteAllText("$scratch/gpu.inc",$source.Substring($begin,$end-$begin))
$begin=$source.IndexOf('void launcher_fade(');$end=$source.IndexOf('void launcher_present(',$begin)
[IO.File]::WriteAllText("$scratch/fade.inc",$source.Substring($begin,$end-$begin))
$begin=$source.IndexOf('void end_frame()');$end=$source.IndexOf('static void updateLiveTownAzelLighting()',$begin)
[IO.File]::WriteAllText("$scratch/end_frame.inc",$source.Substring($begin,$end-$begin))
if(!$source.Contains('if (g_launcherPresentedGxm)')){throw 'Missing duplicate-present guard'}
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/launcher_gxm_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'GPU launcher host compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'GPU launcher host checks failed'}
