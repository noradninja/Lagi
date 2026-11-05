param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$ea,[int]$count){
  $lba=239428L;$base=0x6054000L
  if($ea-lt$base){$lba=58L;$base=0x6006000L}
  $v=0L
  for($i=0;$i-lt$count;$i++){
   $o=$ea+$i-$base;$disc.Position=($lba+[long][Math]::Floor($o/2048))*2352+16+($o%2048)
   $b=$disc.ReadByte();if($b-lt0){throw 'Short native read'};$v=($v-shl8)-bor$b
  };return $v
 }
 foreach($p in @(@(0x606AA22,0xE308),@(0x606AA3A,0xE507),@(0x606AA68,0x315C),
  @(0x606AA6E,0x1234),@(0x606AA98,0x7454),@(0x606AAF8,0x3238),
  @(0x606AB2C,0x2F32),@(0x606AB38,0x1F22),@(0x606AB40,0x123D),
  @(0x606AB54,0x123F),@(0x606ABAC,0x1326),@(0x606ABB2,0x1327),@(0x606ABB8,0x1328),
  @(0x606ABC4,0x53D9),@(0x606ABD8,0x523C),@(0x606ABDC,0x532A),@(0x606ABF2,0xE634))) {
  if((Read-Native $p[0] 2)-ne$p[1]){throw ('Native Camp child instruction differs at{0:X8}'-f$p[0])}
 }
 foreach($p in @(@(0x607EB94,0x606AA04),@(0x607EB98,0x606AB88),@(0x607EB9C,0x606ABD2),@(0x607EBA0,0),
  @(0x606A9F8,0x607EB88),@(0x606A9FC,0xFE5843F2L),@(0x606AB68,0x607EAFC),
  @(0x606AB64,0x6013B4C),@(0x606AB6C,0x6013BE0),@(0x606AB70,0x6013C70),
  @(0x6013C64,0x60136F4),@(0x6013C68,0x60139B8),@(0x6013C6C,0x6013A5C))) {
  if((Read-Native $p[0] 4)-ne$p[1]){throw ('Native Camp child binding differs at{0:X8}'-f$p[0])}
 }
}finally{$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-camp-body-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_camp_npc_body.inc"
[IO.File]::WriteAllText("$scratch/production.inc",[regex]::Replace($source,'(?m)^#include[^\r\n]*',''))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/camp_npc_body_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Camp child fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Camp child fixture failed'}
