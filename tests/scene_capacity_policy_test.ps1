$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-scene-capacity-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$a=$source.IndexOf('    // Reserve the bounded native-scene subdivision capacity')
$b=$source.IndexOf('    // The live field mesh remains a temporary flattened resource', $a)
if($a -lt 0 -or $b -le $a){throw 'Scene capacity boundary changed'}
[IO.File]::WriteAllText("$scratch/capacity_policy.inc",$source.Substring($a,$b-$a))
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/scene_capacity_policy_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Scene capacity fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Scene capacity fixture failed'}
