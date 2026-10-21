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
 function NativeByte([long]$ea){$o=$ea-0x200000;$stream.Position=(182+[long][math]::Floor($o/2048))*2352+16+($o%2048);$b=$stream.ReadByte();if($b -lt 0){throw 'Short COMMON read'};return $b}
 foreach($pair in @(@(0x208748,0),@(0x208749,150),@(0x20874A,120),@(0x20874B,20),
    @(0x207660,8),@(0x207680,8),@(0x207658,5),@(0x207659,50),@(0x20765A,7),@(0x20765B,50),
    @(0x20765C,2),@(0x20765D,10),@(0x20765E,4),@(0x20765F,90))){
    if((NativeByte $pair[0]) -ne $pair[1]){throw ('Native encounter metadata differs at {0:X8}' -f $pair[0])}
 }
 Write-Output 'PASS original COMMON field-2 initial/reset/max-attempt values, 8-percent checks and weighted 5/7, 2/4 formation IDs'
}finally{$stream.Dispose()}
