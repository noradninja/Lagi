param([string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-encounter-cadence-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/field_fieldEncounterTask.cpp"
$a=$source.IndexOf('s32 getEncounterArea(sEncouterTask* pThis)')
$b=$source.IndexOf('#include "lagi/battle_graphics.h"',$a)
if($a -lt 0 -or $b -le $a){throw 'Encounter evaluator boundary changed'}
[IO.File]::WriteAllText("$scratch/encounter_evaluate.inc",$source.Substring($a,$b-$a))
$a=$source.IndexOf('        if (hasEncounterData == 0)')
$b=$source.IndexOf('        pThis->m1 = graphicEngineStatus',$a)
if($a -lt 0 -or $b -le $a){throw 'Encounter enable gate boundary changed'}
[IO.File]::WriteAllText("$scratch/encounter_gate.inc",$source.Substring($a,$b-$a))
$source=Get-Content -Raw "$repo/$Build/azel_upstream/field_field_a3_o_fld_a3.cpp"
$a=$source.IndexOf('bool isBattleEnabled()')
$b=$source.IndexOf('void createRandomBattleTask(',$a)
if($a -lt 0 -or $b -le $a){throw 'Field encounter policy boundary changed'}
$policy=$source.Substring($a,$b-$a)
[IO.File]::WriteAllText("$scratch/encounter_enable.inc",$policy)
$original=Get-Content -Raw "$repo/extern/Azel/AzelLib/field/field_a3/o_fld_a3.cpp"
$a=$original.IndexOf('bool isBattleEnabled()');$b=$original.IndexOf('void createRandomBattleTask(',$a)
if($a -lt 0 -or $b -le $a -or ($original.Substring($a,$b-$a) -replace '\s','') -cne ($policy -replace '\s','')){throw 'Field encounter enable policy differs from Azel'}
$source=Get-Content -Raw "$repo/src/integration/lagi_task_vita.cpp"
$a=$source.IndexOf('void processTasks(s_task** ppTask)')
$b=$source.IndexOf('static p_workArea attachTask(',$a)
if($a -lt 0 -or $b -le $a){throw 'Task scheduler boundary changed'}
[IO.File]::WriteAllText("$scratch/encounter_scheduler.inc",$source.Substring($a,$b-$a))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/encounter_cadence_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Encounter cadence fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Encounter cadence fixture failed'}
