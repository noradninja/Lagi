param([string]$ScratchRoot='../menu-profile-gate-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$begin=$source.IndexOf('    static unsigned previousPerfMenu =')
$end=$source.IndexOf('        logging::writef(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Production menu profile gate boundary changed'}
[IO.File]::WriteAllText("$scratch/gate.inc",$source.Substring($begin,$end-$begin)+"return true; }`n")
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/menu_profile_gate_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Menu profile gate compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Menu profile gate failed'}
