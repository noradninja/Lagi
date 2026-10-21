$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-nbg0-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
# Swizzle syntax is the only adaptation; execute the production shader math.
$shader=(Get-Content -Raw "$repo/shaders/scene_nbg0.cginc").Replace('.xy','.xy()')
[IO.File]::WriteAllText("$scratch/scene_shader.inc",$shader)
$pipeline=Get-Content -Raw "$repo/src/platform/vita/neptune_scene_nbg0.inc"
if($pipeline.Contains('setStochastic(')){throw 'NBG0 must not use stochastic sampling'}
if(!$pipeline.Contains('SCE_GXM_TEXTURE_FILTER_LINEAR')){throw 'Decoded NBG0 filtering missing'}
& cl /nologo /EHsc /std:c++17 "/I$repo/include" "/I$scratch" "$PSScriptRoot/scene_nbg0_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'NBG0 host fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'NBG0 host fixture failed'}
