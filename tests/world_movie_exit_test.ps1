param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build/world-garil')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try{
 function Read-Native([long]$lba,[long]$base,[long]$ea,[int]$count){
  $n=0L;for($i=0;$i-lt$count;$i++){$offset=$ea+$i-$base;$disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048);$b=$disc.ReadByte();if($b-lt0){throw 'Short disc read'};$n=($n-shl8)-bor$b};return $n
 }
 $giftSlot=Read-Native 58 0x6006000 (0x6027AAC+(0x14-1)*2) 2
 if(0x6027AA8+$giftSlot-ne0x6027DEA){throw 'Native status14 switch target differs'}
 foreach($word in @(
  @(0x6027DEA,0xD329), @(0x6027DEC,0xD22B), @(0x6027DF2,0x201B),
  @(0x6027DF4,0x2300), @(0x6027DF6,0xB98B), @(0x6027DF8,0xE415),
  @(0x6027DFA,0xA209), @(0x6027DFC,0xE302), @(0x6028210,0x2F32)
 )){
  if((Read-Native 58 0x6006000 $word[0] 2)-ne$word[1]){throw 'Native status14 progression differs'}
 }
 if((Read-Native 58 0x6006000 0x6027E90 4)-ne0x0604A3FD-or
    (Read-Native 58 0x6006000 0x6027E9C 4)-ne0x06042F1A-or
    (Read-Native 58 0x6006000 0x6042F1A 1)-ne0x02){throw 'Native status14 gift bit differs'}

 $battleSlot=Read-Native 58 0x6006000 (0x6027AAC+(0x15-1)*2) 2
 if(0x6027AA8+$battleSlot-ne0x6027DFE){throw 'Native status15 switch target differs'}
 foreach($word in @(
  @(0x6027DFE,0xD31A), @(0x6027E00,0xD227), @(0x6027E06,0x201B),
  @(0x6027E08,0x2300), @(0x6027E0A,0xB981), @(0x6027E0C,0xE45A),
  @(0x6027E0E,0xA1FF), @(0x6027E10,0xE300)
 )){
  if((Read-Native 58 0x6006000 $word[0] 2)-ne$word[1]){throw 'Native status15 progression differs'}
 }
 if((Read-Native 58 0x6006000 0x6027E68 4)-ne0x0604A403-or
    (Read-Native 58 0x6006000 0x6027EA0 4)-ne0x06042F17-or
    (Read-Native 58 0x6006000 0x6042F17 1)-ne0x10-or
    (Read-Native 182 0x200000 (0x212EAC+0x5A*2) 2)-ne0x0203){
    throw 'Native status15 story flag or Camp setup differs'
 }

 # Status 15 selects the original B2 battle before Camp 5A.
 if((Read-Native 182 0x200000 0x212F9E 2)-ne0x0400-or
    (Read-Native 182 0x200000 (0x2005DC+4*0x14+4) 4)-ne0x2012C4){
    throw 'Native gift battle overlay or sub-battle differs'
 }
 $expectedBattle='BTL_B2.PRG'
 for($i=0;$i-lt$expectedBattle.Length;$i++){
    if((Read-Native 182 0x200000 (0x2012C4+$i) 1)-ne[int]$expectedBattle[$i]){
        throw 'Native gift battle overlay filename differs'
    }
 }
 foreach($pair in @(
    @(0x06054054,0x06054034), @(0x06054058,0x0605400A),
    @(0x0605406C,0x060A3BC4), @(0x06054078,0x060A415C),
    @(0x060A415C,0x060A4060), @(0x060A4160,0x060A4078),
    @(0x060A4164,0x06054A0E), @(0x060A4168,0x060A3FBC),
    @(0x060A4060,0x06054C88), @(0x060A4064,0), @(0x060A4068,1),
    @(0x060A406C,0), @(0x06054A34,0x060A46B8), @(0x06054A38,0x0602FB9E),
    @(0x060A46B8,0x06054374), @(0x060A46BC,0x0605415C),
    @(0x060A46C0,0x060547E8), @(0x060A46C4,0),
    @(0x060A482C,0), @(0x060A4830,0x06054CF2),
    @(0x060A4834,0), @(0x060A4838,0x06054DA2)
 )){
    if((Read-Native 259485 0x06054000 $pair[0] 4)-ne$pair[1]){
        throw 'Native B2 entry callbacks or data references differ'
    }
 }

 $slot=Read-Native 58 0x6006000 (0x6027AAC+(0x54-1)*2) 2
 if(0x6027AA8+$slot-ne0x6027C7A){throw 'Native status54 switch target differs'}
 foreach($status in @(0x55,0x56,0x5A)) {
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
