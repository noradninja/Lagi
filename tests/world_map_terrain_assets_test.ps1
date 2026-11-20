param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-terrain-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$shader=Get-Content -Raw "$repo/shaders/vdp2_rbg0_f.cg"
$a=$shader.IndexOf('float selectRbg0Plane(')
$b=$shader.IndexOf('#if LAGI_RBG_DECODED_PAGES',$a)
$c=$shader.IndexOf('float4 sampleRbg0Cell(')
$d=$shader.IndexOf('float readRbgCoefficient(',$c)
if($a-lt0-or$b-le$a-or$c-lt0-or$d-le$c){throw 'WORLD generic shader anchors changed'}
$cell=($shader.Substring($a,$b-$a)+$shader.Substring($c,$d-$c)).Replace('uniform ','')
[IO.File]::WriteAllText("$scratch/world_terrain_shader.inc",$cell)
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/world_map_terrain_assets_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'WORLD terrain asset fixture compile failed'}
& "$scratch/test.exe" (Resolve-Path -LiteralPath $DiscImage).Path
if($LASTEXITCODE){throw 'WORLD terrain asset fixture failed'}
