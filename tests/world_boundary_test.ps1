param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/kernel_moduleManager.cpp"
if(!$generated.Contains('lagiLoadNativeWorldBoundary, // 6: native WORLD.PRG')){throw 'Native mode 6 handoff missing'}
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$lba,[long]$base,[long]$ea,[int]$count){
  $out=New-Object byte[] $count
  for($i=0;$i -lt $count;$i++){$offset=$ea+$i-$base;$disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'};$out[$i]=$b};return ,$out
 }
 # Locked selection plays the original error sound instead of silently doing nothing.
 foreach($word in @(@(0x06058036,'89-05'),@(0x06058044,'D3-21'),
                    @(0x06058046,'43-0B'),@(0x06058048,'E4-05'))) {
  if([BitConverter]::ToString((Read-Native 244259 0x06054000 $word[0] 2))-ne$word[1]){
   throw 'Native WORLD locked-destination feedback differs'
  }
 }
 $mode=Read-Native 182 0x200000 (0x212EAC+0x4F*2) 2
 $entry=Read-Native 182 0x200000 (0x212FC4+6*4) 4
 $name=([Text.Encoding]::ASCII.GetString((Read-Native 58 0x6006000 0x604382C 10))).Split([char]0)[0]
 if($mode[0] -ne 6 -or $mode[1] -ne 0 -or [BitConverter]::ToString($entry) -ne '06-02-78-78' -or $name -ne 'WORLD.PRG'){throw 'Native WORLD dispatch contract differs'}
 foreach($word in @(@(0x06057ED0,'E0-4D'),@(0x06057ED2,'01-CC'),
  @(0x06057ED4,'E0-4C'),@(0x06057ED6,'03-CC'),@(0x06057ED8,'31-33'),
  @(0x06057EDA,'8B-04'),@(0x06057EE0,'04-B5'),@(0x06057EE4,'04-B5'))) {
  if([BitConverter]::ToString((Read-Native 244259 0x06054000 $word[0] 2))-ne$word[1]){
   throw 'Native WORLD signed fade-group reset differs'
  }
 }
 'PASS original status 4F -> mode 6 -> 06027878 -> WORLD.PRG; no story-status substitution'
} finally {$disc.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-boundary-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_world_boundary_vita.cpp"
$source=[regex]::Replace($source,'(?m)^#include[^\r\n]*','')
[IO.File]::WriteAllText("$scratch/production.inc",$source)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/include" /FIlagi/world_map_environment.h "$PSScriptRoot/world_boundary_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'World boundary fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'World boundary fixture failed'}
