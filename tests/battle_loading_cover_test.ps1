$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-battle-cover-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_render_bridge.cpp"
$a=$source.IndexOf('    // Retain immutable geometry even if the producer retires')
$b=$source.IndexOf('    ++g_publishedFrameNumber;',$a)
if($a -lt 0 -or $b -le $a){throw 'Immutable frame publication boundary changed'}
$b+='    ++g_publishedFrameNumber;'.Length
[IO.File]::WriteAllText("$scratch/publication.inc",$source.Substring($a,$b-$a))
$runtime=Get-Content -Raw "$repo/src/integration/lagi_runtime.cpp"
if(!$runtime.Contains('if (nativeSceneFrame && !retainBattleLoadingScene)') -or
 !$runtime.Contains('lagi::azel_bridge::publish_frame(retainBattleLoadingScene);')){throw 'Loading cover must preserve camera state and immutable scene at existing slot-owned publication'}
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/battle_loading_cover_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Loading-cover fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Loading-cover fixture failed'}
