param([string]$ScratchRoot='../battle-resource-ready-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_disc_file_vita.cpp"
$begin=$source.IndexOf('namespace {');$end=$source.IndexOf('} // namespace',$begin)+14
$helpers=$source.Substring($begin,$end-$begin)
$begin=$source.IndexOf('int loadFile(const char*');$end=$source.IndexOf('sSaturnMemoryFile::sSaturnMemoryFile',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Production file service boundary changed'}
[IO.File]::WriteAllText("$scratch/service.inc",$helpers+"`n"+$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_resource_ready_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Resource-ready test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Resource readiness test failed'}
