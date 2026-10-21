$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/build-battle-dev/azel_upstream/battle_BTL_A3_2_Arachnoth.cpp"
if(!$source.Contains("void arachnoth_enterDeathState(sArachnothFormation* pThis)`n{`n    arachnoth_disableAllTargetables(pThis);")){
 $normalized=$source.Replace("`r",'')
 if(!$normalized.Contains("void arachnoth_enterDeathState(sArachnothFormation* pThis)`n{`n    arachnoth_disableAllTargetables(pThis);")){throw 'Death entry does not retire targetables'}
}
$begin=$source.IndexOf('static void arachnoth_disableSubModelTargetables(sArachnothSubModel* pThis)')
$end=$source.IndexOf('// 06058f2e',$begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Target cleanup helpers absent'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-target-retire-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/boss_target_retirement_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Target retirement fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Target retirement fixture failed'}
$target=Get-Content -Raw "$repo/extern/Azel/AzelLib/battle/battleTargetable.cpp"
if(!$target.Contains('if (pThis->m84_pTargetable->m50_flags & 0x40000)')){throw 'Generic indicator retirement contract changed'}
