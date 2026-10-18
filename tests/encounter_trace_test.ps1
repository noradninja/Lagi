param([string]$ScratchRoot='../encounter-trace-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/src/integration" "/I$repo/include" "$PSScriptRoot/encounter_trace_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Encounter trace test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Encounter trace test failed'}
# Strip only the observer calls and diagnostic lines. All remaining native
# area selection, probability, RNG and counter code must match upstream.
$original=(Get-Content -Raw "$repo/extern/Azel/AzelLib/field/fieldEncounterTask.cpp").Replace("`r`n","`n")
$generated=(Get-Content -Raw "$repo/$BattleBuild/azel_upstream/field_fieldEncounterTask.cpp").Replace("`r`n","`n")
$start='s32 getEncounterArea(sEncouterTask* pThis)'
$a=$original.IndexOf($start);$b=$original.IndexOf('// 0602849a',$a)
$c=$generated.IndexOf($start);$d=$generated.IndexOf('#include "lagi/battle_graphics.h"',$c)
if($a -lt 0 -or $b -le $a -or $c -lt 0 -or $d -le $c){throw 'Encounter comparison boundaries changed'}
$native=$original.Substring($a,$b-$a)
$observed=$generated.Substring($c,$d-$c)
$observed=[regex]::Replace($observed,'(?m)^\s*lagiReportEncounterState\([^\n]+\);\n','')
$observed=[regex]::Replace($observed,'(?m)^\s*lagi::platform::logging::writef\("\[NativeEncounterRoll\][^\n]+\);\n','')
if(($native -replace '\s','') -cne ($observed -replace '\s','')){throw 'Diagnostics changed native encounter evaluation code'}
Write-Output 'PASS: native encounter area/probability/RNG/counter code is unchanged after removing observer calls.'
