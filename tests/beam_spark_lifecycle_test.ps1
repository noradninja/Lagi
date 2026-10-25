param([string]$ScratchRoot='../beam-spark-lifecycle-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/field_fieldBeamSpark.cpp"
if(!$source.Contains('pThis->m_DrawMethod = &beamSpark_UpdateAlt;') -or $source.Contains('pThis->m_UpdateMethod = &beamSpark_UpdateAlt;')){throw 'Generated native spark callback ownership incorrect'}
$start=$source.IndexOf('// Beam spark effect')
if($start-lt 0){throw 'Generated spark body boundary changed'}
[IO.File]::WriteAllText("$scratch/spark_service.inc",$source.Substring($start))
$header=(Get-Content -Raw "$repo/extern/Azel/AzelLib/field/fieldBeamSpark.h").Replace('#pragma once','')
[IO.File]::WriteAllText("$scratch/spark_header.inc",$header)
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/beam_spark_lifecycle_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Beam spark lifecycle compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Beam spark lifecycle assertions failed'}
