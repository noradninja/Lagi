param([string]$ScratchRoot='../battle-status-light-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$native=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_battleDragon.cpp"
$begin=$native.IndexOf('    if (local_24)')
$end=$native.IndexOf('    return local_24;',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Status-light envelope boundary changed'}
[IO.File]::WriteAllText("$scratch/status.inc",'void run(Task* pThis,int currentIndex,int local_24){'+$native.Substring($begin,$end-$begin)+'}')
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_status_light_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Status-light test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Status-light test failed'}
