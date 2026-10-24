param([string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-camera-cut-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/excavation_camera_cut_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Camera cut fixture compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Camera cut fixture failed'}
$source=Get-Content -Raw "$repo/$Build/azel_upstream/field_fieldCutsceneTask2.cpp"
$service=(Get-Content -Raw "$repo/src/integration/lagi_excavation_camera_cut.inc").Replace("`r`n","`n").Trim()
if(!$source.Replace("`r`n","`n").Contains($service)){throw 'Generated camera cut service stale'}
$callback=$source.IndexOf('void s_cutsceneTask2::UpdateSub1()')
$cut=$source.IndexOf('    lagiCutCompletedExcavationCamera();',$callback)
$restore=$source.IndexOf('        endCutsceneCameraNoRestore();',$callback)
if($callback -lt 0 -or $cut -lt $callback -or $restore -lt $cut){throw 'Camera cut must precede restoration'}
'PASS production camera cut precedes flight-camera restoration'
