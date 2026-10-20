param([string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-exit-fade-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/module_exit_fade_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Exit fade fixture compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Exit fade fixture failed'}
$generated=Get-ChildItem "$repo/$Build" -Recurse -Filter '*moduleManager.cpp' | Select-Object -First 1
if(!$generated){throw 'Generated module manager missing'}
$source=Get-Content -Raw $generated.FullName
$service=(Get-Content -Raw "$repo/src/integration/lagi_module_exit_fade.inc").Replace("`r`n","`n").Trim()
if(!$source.Replace("`r`n","`n").Contains($service)){throw 'Generated fade service is stale'}
$gate=$source.IndexOf('pWorkArea->state == 0 && lagiWaitForModuleExitFade')
$delete=$source.IndexOf('pWorkArea->m8->getTask()->markFinished()', $gate)
if($gate -lt 0 -or $delete -lt $gate){throw 'Production fade gate not before destructive transition'}
Write-Output 'PASS generated module manager gate precedes outgoing task deletion'
