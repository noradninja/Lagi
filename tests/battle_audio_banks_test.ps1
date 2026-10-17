param([string]$ScratchRoot='../battle-audio-banks-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_audio_vita.cpp"
$begin=$source.IndexOf('bool load_sound_file_list(');$end=$source.IndexOf('bool load_current_banks()',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Sound-bank loader boundary changed'}
[IO.File]::WriteAllText("$scratch/load.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc /DLAGI_BATTLE_AUDIO_STACK=1 "/I$scratch" "/I$repo/include" "$PSScriptRoot/battle_audio_banks_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Sound-bank loader test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Sound-bank loader test failed'}
