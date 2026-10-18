param([string]$Build='build-battle-dev',[string]$ScratchRoot='../native-remainder-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/mainMenuDebugTasks.cpp"
$begin=$source.IndexOf('u32 performModulo(');$end=$source.IndexOf('void setOpenMenu7()',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native remainder fixture boundary changed'}
[IO.File]::WriteAllText("$scratch/remainder_body.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /EHsc /std:c++20 "/I$scratch" "$PSScriptRoot/native_remainder_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native remainder fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Native remainder fixture failed'}
