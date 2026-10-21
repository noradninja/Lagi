param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rng-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
function RngBody($s){$a=$s.IndexOf('u32 RNG_seed = 0;');$b=$s.IndexOf('bool readKeyboardToggle',$a);if($a -lt 0 -or $b -le $a){throw 'RNG source boundary changed'};$s.Substring($a,$b-$a)}
$generated=RngBody (Get-Content "$repo/$Build/azel_upstream/dummy.cpp" -Raw)
$upstream=RngBody (Get-Content "$repo/extern/Azel/AzelLib/dummy.cpp" -Raw)
if(($generated -replace '\s','') -cne ($upstream -replace '\s','')){throw 'Generated RNG differs from Azel'}
[IO.File]::WriteAllText("$scratch/rng_body.inc",$generated)
& cl /nologo /O2 /EHsc /std:c++20 "/I$scratch" "$PSScriptRoot/encounter_rng_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'RNG fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'RNG fixture failed'}
$stream=[IO.File]::OpenRead($DiscImage)
try {
 function MainWord([long]$ea){
  $v=0
  for($i=0;$i -lt 2;++$i){$o=$ea+$i-0x6006000;$stream.Position=(58+[long][math]::Floor($o/2048))*2352+16+($o%2048);$b=$stream.ReadByte();if($b -lt 0){throw 'Short MAIN read'};$v=($v-shl 8)-bor $b};return $v
 }
 # Original seed initializer at 060361E8 enters the same routine at F8,
 # passing R4 through R0; ordinary randomNumber starts at 060361F4.
 # Check the complete arithmetic/branch/store sequence, not only its constant.
 $words=@(0xD309,0x6032,0x610D,0x2118,0x8F01,0xE129,0xD007,0x0107,
    0x4029,0x011A,0x621D,0x4129,0x312C,0x4128,0x201B,0x6009,0x212B,0x000B,0x2312)
 for($i=0;$i -lt $words.Count;++$i){if((MainWord (0x60361F4+$i*2)) -ne $words[$i]){throw ('Original RNG instruction differs at {0:X8}' -f (0x60361F4+$i*2))}}
 foreach($pair in @(@(0x60361E8,0xD301),@(0x60361EA,0xA005),@(0x60361EC,0x6043),
    @(0x60361F0,0x0604),@(0x60361F2,0xB02C),@(0x603621C,0x0604),@(0x603621E,0xB02C),
    @(0x6036220,0x2A6D),@(0x6036222,0x365B))){if((MainWord $pair[0]) -ne $pair[1]){throw 'Original RNG seed/literal contract differs'}}
 Write-Output 'PASS original MAIN seed initializer, complete RNG arithmetic/branch/store sequence and zero-low-word constant'
 # Encounter update decrements the native count once, stores it in the delay
 # slot, checks unsigned roll < chance, increments failed attempts, compares
 # against the maximum, and reloads the signed 16-bit reset counter.
 foreach($pair in @(@(0x600CB3E,0x52D4),@(0x600CB40,0x72FF),@(0x600CB42,0x4215),
    @(0x600CB44,0x8D22),@(0x600CB46,0x1D24),@(0x600CB4C,0x68E0),
    @(0x600CB54,0xE064),@(0x600CB56,0x3806),@(0x600CB58,0x8B07),
    @(0x600CB6A,0x84DE),@(0x600CB6C,0x7001),@(0x600CB70,0x80DE),
    @(0x600CB72,0x84DF),@(0x600CB74,0x3303),@(0x600CB76,0x8B07),
    @(0x600CB88,0x85DB),@(0x600CB8A,0x1D04),@(0x600CBEC,0x0603),@(0x600CBEE,0x61F4),
    @(0x600CBF0,0x0603),@(0x600CBF2,0x5550))){
    if((MainWord $pair[0]) -ne $pair[1]){throw ('Original encounter counter/probability instruction differs at {0:X8}' -f $pair[0])}
 }
 Write-Output 'PASS original encounter countdown, unsigned probability, failure cap, reset and RNG/modulo call literals'
 function NativeByte([long]$ea){$o=$ea-0x200000;$stream.Position=(182+[long][math]::Floor($o/2048))*2352+16+($o%2048);$b=$stream.ReadByte();if($b -lt 0){throw 'Short COMMON read'};return $b}
 foreach($pair in @(@(0x208748,0),@(0x208749,150),@(0x20874A,120),@(0x20874B,20),
    @(0x207660,8),@(0x207680,8),@(0x207658,5),@(0x207659,50),@(0x20765A,7),@(0x20765B,50),
    @(0x20765C,2),@(0x20765D,10),@(0x20765E,4),@(0x20765F,90))){
    if((NativeByte $pair[0]) -ne $pair[1]){throw ('Native encounter metadata differs at {0:X8}' -f $pair[0])}
 }
 Write-Output 'PASS original COMMON field-2 initial/reset/max-attempt values, 8-percent checks and weighted 5/7, 2/4 formation IDs'
}finally{$stream.Dispose()}
