param([string]$ScratchRoot='../battle-resident-video-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_resident_video_vita.cpp"
$begin=$source.IndexOf('namespace lagi::resident_video {')
if($begin -lt 0){throw 'Resident service boundary changed'}
[IO.File]::WriteAllText("$scratch/resident.inc",$source.Substring($begin))
$source=Get-Content -Raw "$repo/src/integration/lagi_disc_file_vita.cpp"
$begin=$source.IndexOf('namespace {');$end=$source.IndexOf('} // namespace',$begin)+14
$helpers=$source.Substring($begin,$end-$begin)
$begin=$source.IndexOf('int loadFile(const char*');$end=$source.IndexOf('sSaturnMemoryFile::sSaturnMemoryFile',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'File service boundary changed'}
[IO.File]::WriteAllText("$scratch/file_service.inc",$helpers+"`n"+$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc /DLAGI_TRACK_RESIDENT_VIDEO=1 "/I$scratch" "/I$repo/include" "$PSScriptRoot/battle_resident_video_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Resident video test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Resident video test failed'}
