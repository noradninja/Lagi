$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-append-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$current=Get-Content "$repo/src/platform/vita/neptune_renderer.cpp" -Raw
$baseline=(& git -C $repo show '92470e9:src/platform/vita/neptune_renderer.cpp') -join "`n"
if($LASTEXITCODE -ne 0){throw 'Accepted baseline source unavailable'}
function Slice($source,$begin,$end){
 $a=$source.IndexOf($begin);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Production extraction boundary changed: $begin"}
 $source.Substring($a,$b-$a)
}
$light=Slice $current 'struct LivePolygonLightState {' 'static std::vector<LivePolygonLightState>'
$transform=Slice $current 'static void transformLiveLightVectorToModelSpace(' 'static void appendLiveTownModel('
$body=Slice $current 'static void appendLiveTownModel(' 'static void appendLiveTownEdge()'
$reference=(Slice $baseline 'static void appendLiveTownModel(' 'static void appendLiveTownEdge()').Replace('appendLiveTownModel(','appendLiveTownModelReference(')
[IO.File]::WriteAllText("$scratch/append_types.inc",$light)
[IO.File]::WriteAllText("$scratch/append_body.inc",$transform+$body+$reference)
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/live_model_append_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Production model append fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Production model append fixture failed'}
