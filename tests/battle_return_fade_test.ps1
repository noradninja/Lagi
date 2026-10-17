param([string]$ScratchRoot='../battle-return-fade-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$native=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/field_fieldEncounterTask.cpp"
$begin=$native.IndexOf('    case 6: // final cleanup')
$end=$native.IndexOf('    default:',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native encounter resume boundary changed'}
[IO.File]::WriteAllText("$scratch/resume.inc", "void resume(sEncouterTask* pThis){switch(pThis->m0_status){`n"+$native.Substring($begin,$end-$begin)+"default:break;}}")
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_return_fade_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Return fade test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Return fade test failed'}
