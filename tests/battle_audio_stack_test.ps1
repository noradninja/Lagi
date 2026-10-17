param([string]$ScratchRoot='../battle-audio-stack-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_audio_vita.cpp"
foreach($part in @(@('void worker_load_banks(','void worker_fade_all()','load.inc'),@('void worker_pop_sequence(','void process_audio_events()','pop.inc'),@('void replay_restored_sequence()','bool advance_loading_state(','replay.inc'),@('bool push_audio_event(','void reset_active_sounds()','queue.inc'),@('void process_audio_events()','int audio_worker_thread(','events.inc'),@('bool isSoundLoadingFinished()','void popSoundSequence(','ready.inc'))){
 $begin=$source.IndexOf($part[0]);$end=$source.IndexOf($part[1],$begin)
 if($begin -lt 0 -or $end -le $begin){throw "Production audio boundary changed: $($part[0])"}
 [IO.File]::WriteAllText((Join-Path $scratch $part[2]),$source.Substring($begin,$end-$begin))
}
& cl /nologo /O2 /std:c++20 /EHsc /DLAGI_BATTLE_AUDIO_STACK=1 "/I$scratch" "/I$repo/include" "$PSScriptRoot/battle_audio_stack_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Battle audio test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Battle audio test failed'}
