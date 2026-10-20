param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$ea,[int]$bytes,[long]$lba=258144,[long]$base=0x06054000){
  $value=0L
  for($i=0;$i -lt $bytes;$i++){
   $offset=$ea+$i-$base
   $disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $byte=$disc.ReadByte();if($byte -lt 0){throw 'Short disc read'}
   $value=($value-shl 8)-bor $byte
  };return $value
 }
 foreach($pair in @(@(0x0606F23C,0x0020701C,4),@(0x0606F08C,0x0020701C,4),
  @(0x0606F0D6,0x8862,2),@(0x0606F238,0x00008000,4),@(0x0606F062,0x02DE,2),
  @(0x0606F064,0x02F8,2),@(0x0606F066,0x02F0,2),@(0x0606F068,0x02F4,2),@(0x0606F06A,0x02EC,2))) {
  if((Read-Native $pair[0] $pair[2]) -ne $pair[1]){throw ('Native growth contract differs at {0:X8}' -f $pair[0])}
 }
 $previous=0L
 for($level=1;$level -le 98;$level++){
  $threshold=Read-Native (0x0020701C+$level*12) 4 182 0x00200000
  if($threshold -lt $previous){throw 'Native thresholds are not monotonic'}
  $previous=$threshold
 }
 'PASS original growth table references, native rounding/counter addresses and 98 threshold rows'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-growth-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
foreach($test in @('battle_growth_test','save_growth_migration_test')){
 & cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/$test.cpp" "/Fe:$scratch/$test.exe" "/Fo:$scratch/$test.obj"
 if($LASTEXITCODE -ne 0){throw 'Growth fixture compile failed'}
 & "$scratch/$test.exe"
 if($LASTEXITCODE -ne 0){throw 'Growth fixture failed'}
}
