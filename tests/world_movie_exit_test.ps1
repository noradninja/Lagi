param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build/world-garil')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try{
 function Read-Native([long]$lba,[long]$base,[long]$ea,[int]$count){
  $n=0L;for($i=0;$i-lt$count;$i++){$offset=$ea+$i-$base;$disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b-lt0){throw 'Short disc read'};$n=($n-shl8)-bor$b};return $n
 }
 $slot=Read-Native 58 0x6006000 (0x6027AAC+(0x54-1)*2) 2
 if(0x6027AA8+$slot-ne0x6027C7A){throw 'Native status54 switch target differs'}
 foreach($status in @(0x55,0x56)) {
  $target=0x6027AA8+(Read-Native 58 0x6006000 (0x6027AAC+($status-1)*2) 2)
  if($target-ne0x60283F0){throw 'Native camp/Garil return target differs'}
 }
 foreach($pair in @(@(0x60283F0,0xE44F),@(0x60283F2,0xD345),@(0x60283F4,0x430B),@(0x60283F6,0x0009))) {
  if((Read-Native 58 0x6006000 $pair[0] 2)-ne$pair[1]){throw 'Native WORLD return instructions differ'}
 }
 if((Read-Native 58 0x6006000 0x6028508 4)-ne0x6027110){throw 'Native return status service differs'}
 foreach($word in @(@(0x6027C84,0x2300),@(0x6027C90,0x2100),@(0x6027C9A,0x2100),@(0x6027CA4,0x2100),@(0x6027CA6,0xA2FC),@(0x6027CA8,0xE455),@(0x60282A2,0xD242),@(0x60282A4,0x420B))){
  if((Read-Native 58 0x6006000 $word[0] 2)-ne$word[1]){throw 'Native status54 mutation/next-status instructions differ'}
 }
 foreach($pair in @(@(0x6042F1B,1),@(0x6042F1E,0xDF),@(0x6042F18,8),@(0x6042F17,0x10))){
  if((Read-Native 58 0x6006000 $pair[0] 1)-ne$pair[1]){throw 'Native story mask differs'}
 }
 if((Read-Native 182 0x200000 (0x212EAC+0x55*2) 2)-ne0x0201){throw 'Status55 no longer resolves town pair1'}
 # Tie the production movie-exit mutation to WORLD's actual Garil gate,
 # rather than assuming camp completion or another unlock is required.
 if((Read-Native 244259 0x6054000 (0x6059688+6*0x24) 2)-ne(13*8+4)){
  throw 'Garil gate no longer matches the native Cainus arrival mutation'
 }
 $overlay=Read-Native 182 0x200000 0x2166E6 1
 $name=Read-Native 182 0x200000 (0x2165D8+$overlay*16) 4
 $expected='TWN_CAMP.PRG'
 for($i=0;$i-lt$expected.Length;$i++){
  if((Read-Native 182 0x200000 ($name+$i) 1)-ne[int]$expected[$i]){throw 'Native status55 overlay is not TWN_CAMP'}
 }
}finally{$disc.Dispose()}
$service=Get-Content -Raw "$repo/src/integration/lagi_world_movie_exit.inc"
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/kernel_moduleManager.cpp"
if(!$generated.Replace("`r`n","`n").Contains($service.Replace("`r`n","`n").Trim())){throw 'Generated WORLD movie-exit adapter is stale'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-world-movie-exit-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",$service)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/world_movie_exit_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE){throw 'Movie exit fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE){throw 'Movie exit fixture failed'}
