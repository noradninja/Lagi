param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build/world-garil')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Main([long]$ea,[int]$count){$v=0L;for($i=0;$i-lt$count;$i++){$o=$ea+$i-0x6006000;$disc.Position=(58+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b-lt0){throw 'Short MAIN read'};$v=($v-shl8)-bor$b};return $v}
 foreach($pair in @(@(0x6011EF4,0x015D),@(0x6011EF6,0x3140),@(0x6011EF8,0x8916),
  @(0x6011EFC,0x8803),@(0x6011F00,0x8806),@(0x6011F04,0x8811),
  @(0x6011F0E,0xEE56),@(0x6011F12,0xEE63),@(0x6011F16,0xEE6D),
  @(0x6011F18,0x9550),@(0x6011F1A,0x67E3),@(0x6011F1E,0xE606),
  @(0x6011F22,0x77B0),@(0x6011F24,0x432B),@(0x6011FBC,0x0087))) {
  if((Read-Main $pair[0] 2)-ne$pair[1]){throw 'Native field route conversion differs'}
 }
 if((Read-Main 0x6011FCC 4)-ne0x604A3F8-or(Read-Main 0x6011FD0 4)-ne0x60132C8){throw 'Native route bit writer differs'}
}finally{$disc.Dispose()}
$source=Get-Content -Raw "$repo/$Build/azel_upstream/field.cpp"
$start=$source.IndexOf('void exitCutsceneTaskUpdateSub0Sub1Sub0Sub0(')
$end=$source.IndexOf('void exitCutsceneTaskUpdateSub0Sub1Sub0Sub1(',$start)
if($start-lt0-or$end-le$start){throw 'Generated field route helper boundary changed'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-field-world-route-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",$source.Substring($start,$end-$start))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/field_world_route_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Field route fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Field route fixture failed'}
