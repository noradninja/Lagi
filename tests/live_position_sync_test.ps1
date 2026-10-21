param([string]$ScratchRoot='../live-position-sync-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$begin=$source.IndexOf('        const std::size_t firstDynamicVertex =',$source.IndexOf('// LAGI_LIVE_POSITION_SYNC_BEGIN'))
$end=$source.IndexOf('        // LAGI_LIVE_POSITION_SYNC_END',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Live position sync fixture boundary changed'}
$body=$source.Substring($begin,$end-$begin)
if($body.Contains('g_vdp1LightingVertices[')){throw 'Unused live lighting buffer is copied again'}
[IO.File]::WriteAllText("$scratch/live_position_sync.inc",$body)
& cl /nologo /O2 /EHsc /std:c++20 "/I$scratch" "$PSScriptRoot/live_position_sync_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Live position sync fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Live position sync fixture failed'}
# The only reference inside the complete draw implementation must remain the
# allocation-readiness check, not a stream/content consumer. Re-audit if changed.
$submit=$source.IndexOf('bool submit_vdp1_model(')
$next=$source.IndexOf('static bool clearNativeSceneColorBuffer(', $submit)
if($submit -lt 0 -or $next -le $submit){throw 'VDP1 submission audit boundary changed'}
$refs=[regex]::Matches($source.Substring($submit,$next-$submit),'g_vdp1LightingVertices')
if($refs.Count -ne 1){throw 'Legacy lighting buffer gained a consumer; re-audit dead-copy removal'}
