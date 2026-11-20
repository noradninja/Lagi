$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-visibility-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$current=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$prior=(& git -C $repo show b90d201:src/platform/vita/neptune_renderer.cpp) -join "`n"
if($LASTEXITCODE -ne 0){throw 'Visibility baseline unavailable'}
function Slice($text,$first,$last){
 $a=$text.IndexOf($first);$b=$text.IndexOf($last,$a)
 if($a -lt 0 -or $b -le $a){throw "Visibility boundary changed: $first"}
 $text.Substring($a,$b-$a)
}
$types=Slice $current 'struct ViewerScreenPoint' 'static int viewerRenderWidth()'
$body=Slice $current 'static void prepareLiveTownGouraudVisibility(' 'static void generateAzelFalloff('
$baseline=(Slice $prior 'static void prepareLiveTownGouraudVisibility(' 'static void generateAzelFalloff(').Replace('prepareLiveTownGouraudVisibility(','prepareReference(')
# Exactly one consumer copies screen payload, and must request lazy projection.
if(([regex]::Matches($current,'std::memcpy\(screen, prep.screen')).Count -ne 1 -or
 !$current.Contains("projectLiveTownGouraudScreen(wvp, p, prep);")){throw 'Screen payload consumer changed; re-audit lazy projection'}
[IO.File]::WriteAllText("$scratch/visibility.inc",$types+$body+$baseline)
& cl /nologo /O2 /fp:strict /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/live_visibility_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Visibility fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Visibility fixture failed'}
