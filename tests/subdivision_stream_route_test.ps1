$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-sub-stream-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$a=$source.IndexOf('static bool liveSubdivisionBuffersReady(')
$b=$source.IndexOf('static bool buildLiveTownFrame()', $a)
if($a -lt 0 -or $b -le $a){throw 'Shared readiness boundary changed'}
[IO.File]::WriteAllText("$scratch/subdivision_ready.inc",$source.Substring($a,$b-$a))
$a=$source.IndexOf('        const bool subdivisionOnly =')
$b=$source.IndexOf('        // LAGI_LIVE_POSITION_SYNC_BEGIN', $a)
if($a -lt 0 -or $b -le $a){throw 'Stream route boundary changed'}
[IO.File]::WriteAllText("$scratch/subdivision_route.inc",$source.Substring($a,$b-$a))
$a=$source.IndexOf('    const bool subdividedTexturedLit =')
$b=$source.IndexOf('    const bool subdividedGouraudGray =', $a)
if($a -lt 0 -or $b -le $a -or !$source.Substring($a,$b-$a).Contains('liveSubdivisionBuffersReady(model)')){throw 'Draw no longer uses shared readiness predicate'}
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/subdivision_stream_route_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Subdivision stream fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Subdivision stream fixture failed'}
