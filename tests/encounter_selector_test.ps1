param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-selector-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/field_fieldEncounterTask.cpp"
$a=$source.IndexOf('        sSaturnPtr pRandomEncounterData =')
$b=$source.IndexOf('        pThis->m0_status = 1;', $a)
if($a -lt 0 -or $b -le $a){throw 'Generated initializer boundary changed'}
$b+='        pThis->m0_status = 1;'.Length
[IO.File]::WriteAllText("$scratch/selector_init.inc",$source.Substring($a,$b-$a))
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/common.h"
$a=$source.IndexOf('    bool getBit(u32 bitIndex)')
$b=$source.IndexOf('    bool getBit(u32 byteIndex, u8 mask)', $a)
if($a -lt 0 -or $b -le $a){throw 'Actual game-state bit reader boundary changed'}
[IO.File]::WriteAllText("$scratch/get_bit.inc",$source.Substring($a,$b-$a))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/encounter_selector_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Selector fixture compile failed'}
& "$scratch/test.exe" $DiscImage
if($LASTEXITCODE -ne 0){throw 'Selector fixture failed'}
