param([string]$BuildDirectory="$PSScriptRoot/../build-battle-dev")
$ErrorActionPreference='Stop'
$source=Get-Content -Raw "$BuildDirectory/azel_upstream/battle_BTL_A3_2_Arachnoth.cpp"
$source=$source.Replace("`r`n","`n")
$start=$source.IndexOf('static void arachnoth_eatDragonAttack_update(sArachnothFormation* pThis)'+"`n{")
if($start -lt 0){throw 'Generated Devour dispatcher missing'}
$end=$source.IndexOf('void arachnoth_idle_finish', $start)
if($end -lt 0){throw 'Generated dispatcher boundary missing'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-devour-dispatch-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/devour_dispatch.inc",$source.Substring($start,$end-$start))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/arachnoth_devour_dispatch_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Dispatcher fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Dispatcher fixture failed'}
