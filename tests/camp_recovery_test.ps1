param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Camp([long]$ea,[int]$count){$v=0L;for($i=0;$i-lt$count;$i++){$o=$ea+$i-0x6054000;$disc.Position=(239428+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$b=$disc.ReadByte();if($b-lt0){throw 'Short CAMP read'};$v=($v-shl8)-bor$b};return $v}
 foreach($pair in @(@(0x60555E4,0x9019),@(0x60555E6,0x4319),@(0x60555E8,0x0454),
  @(0x60555EE,0x4529),@(0x60555FA,0x0464),@(0x605560A,0x0436),@(0x6055610,0x0476),
  @(0x605564E,0x2338),@(0x605565E,0xBFC0),@(0x6055674,0x8B06),
  @(0x605568C,0x3323),@(0x6055694,0x7301),@(0x605569C,0xD341),
  @(0x60556AC,0x04EC),@(0x60556B0,0x03EC),@(0x6055704,0x23D9),
  @(0x6055710,0xD326),@(0x6055728,0x7F0C),
  @(0x605586A,0xE31E),@(0x6055876,0xE278),
  @(0x605573C,0x143B),@(0x6055740,0x145C),@(0x6054DE8,0x1E1A),
  @(0x605584C,0x2228),@(0x6055902,0x8D32),
  @(0x60568F6,0x7520),@(0x60568F8,0x5553),@(0x6056904,0x7424),
  @(0x6057140,0x7164),@(0x6057144,0x8B0B),@(0x605714C,0x8B27),
  @(0x605718E,0x6593),@(0x6057196,0x7424),
  @(0x6057372,0xFD71),@(0x6057374,0x0CCC),@(0x605733C,0x53E3),
  @(0x606CA50,0xD334),@(0x606CA6A,0x1514),@(0x606B396,0x000B),@(0x606B398,0x0009))) {
  if((Read-Camp $pair[0] 2)-ne$pair[1]){throw ('Native recovery arithmetic anchor differs at{0:X8}'-f$pair[0])}
 }
 foreach($pair in @(@(0x60558E0,0x1F1F1F),@(0x60557A4,0x6035664),
  @(0x60557A8,0x6035644),@(0x60557AC,0x601DFC8),@(0x60557B0,0x601DF8A),
  @(0x60569D4,0xAAAAAA),@(0x60569DC,0x4000000),@(0x60569E0,0x6073CA8),
  @(0x60569E4,0x6053248),@(0x6057258,0x602E428),
  @(0x6057378,0x607B264),@(0x6057380,0x602CF38),
  @(0x607B270,0),@(0x607B274,0x60569EC),@(0x607B278,0x60570F0),@(0x607B27C,0))) {
  if((Read-Camp $pair[0] 4)-ne$pair[1]){throw 'Native recovery light binding differs'}
 }
}finally{$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-camp-recovery-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/camp_recovery_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Camp recovery fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Camp recovery fixture failed'}
