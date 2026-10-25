param([string]$BuildDirectory='build-battle-dev',[string]$ScratchRoot='../tutorial-movement-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/battle_BTL_T0_BTL_T0.cpp"
$begin=$source.IndexOf('static void tutorialPhase1(sTutorialControllerTask* pThis)')
$end=$source.IndexOf('// 06059464',$begin)
if($begin-lt 0 -or $end-le $begin){throw 'Movement lesson boundary changed'}
$function=$source.Substring($begin,$end-$begin)
$fixture=Get-Content -Raw "$PSScriptRoot/tutorial_movement_test.cpp"
[IO.File]::WriteAllText("$scratch/test.cpp",$fixture.Replace('// GENERATED_MOVEMENT_LESSON',$function))
& cl /nologo /std:c++20 /EHsc "$scratch/test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Movement lesson compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Movement lesson assertions failed'}
Write-Output 'PASS: actual generated movement lesson helper dispatch and simulated modal handshake (helpers mocked); hardware acceptance pending'
