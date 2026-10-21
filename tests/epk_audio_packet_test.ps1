param([string]$Build='build-battle-dev',[string]$ScratchRoot='../epk-audio-packet-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot));New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/town_townCutscene.cpp"
$a=$source.IndexOf('s32 cutsceneCommand1(');$b=$source.IndexOf('s32 cutsceneCommand0(',$a)
if($a -lt 0 -or $b -le $a){throw 'EPK audio packet boundary changed'}
[IO.File]::WriteAllText("$scratch/packet.inc",$source.Substring($a,$b-$a))
if(!$source.Contains('lagiStopCutscenePCM(iParm1);') -or !$source.Contains('lagiPauseCutscenePCM(stream,paused!=0);')){throw 'Audio lifecycle hooks missing'}
$worker=Get-Content -Raw "$repo/src/integration/lagi_audio_vita.cpp"
$mix=$worker.IndexOf('g_cutscenePCM.mix(g_mixBuffer.data()')
if($mix -lt 0 -or $worker.IndexOf('lagi::platform::audio::write_pcm_frames(', $mix) -lt $mix){throw 'PCM mixed-output boundary missing'}
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/epk_audio_packet_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'EPK packet fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'EPK packet fixture failed'}
