param([string]$Build='build-battle-dev',[string]$ScratchRoot='../resource-retirement-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot));New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/kernel_graphicalObject.cpp"
$begin=$source.IndexOf('void npcFileDeleter::Delete(');$end=$source.IndexOf('// equivalent of loadDragonSub1Sub1',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Resource deleter boundary changed'}
[IO.File]::WriteAllText("$scratch/delete.inc",$source.Substring($begin,$end-$begin))
$source=Get-Content -Raw "$repo/$Build/azel_upstream/dragonData.cpp"
$begin=$source.IndexOf('void vdp1Free(u8* ptr)');$end=$source.IndexOf('void loadDragon(s_workArea* pWorkArea)',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'VDP1 free boundary changed'}
[IO.File]::WriteAllText("$scratch/free.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/resource_retirement_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Resource retirement fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Resource retirement fixture failed'}
