param([string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw (Join-Path $repo "$BuildDirectory/azel_upstream/battle_battleResultScreen.cpp")
$start=$source.IndexOf('    case 0:')
$end=$source.IndexOf('    case 1:',$start)
$entry=$source.Substring($start,$end-$start)
if($entry.Contains('0xa94a') -or $entry.Contains('battleGrid_setupLightInterpolation(') -or $entry.Contains('battleResultScreen_updateSub0(-0x1f')){throw 'Results still darken before dismissal'}
if(!$entry.Contains('LAGI_RESULT_HOLD_LIGHTING')){throw 'Result hold adapter absent'}
$start=$source.IndexOf('    case 6:')
$end=$source.IndexOf('    case 7:',$start)
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-result-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/dismissal.inc",$source.Substring($start,$end-$start))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_result_dismissal_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Result fixture compilation failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Result fixture failed'}
