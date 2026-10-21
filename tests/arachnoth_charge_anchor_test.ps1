param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/$Build/azel_upstream/battle_BTL_A3_2_Arachnoth.cpp"
foreach($ea in '0x060A909C','0x060A9090') {
 if(!$source.Contains("tempVec = readSaturnVec3(g_BTL_A3_2->getSaturnPtr($ea))")){throw 'Generated Charge anchor adapter missing'}
}
if($source.Contains('tempVec(0x201000, 0x13000, 0x1D2000)') -or $source.Contains('tempVec(0x201000, 0x13000, 0x230000)')){throw 'Positive-Z Charge anchor retained'}
$disc=[IO.File]::OpenRead($DiscImage)
try {
 foreach($anchor in @(@(0x060A909C,-0x1D2000),@(0x060A9090,-0x230000))) {
  $point=@()
  for($axis=0;$axis -lt 3;$axis++) {
   $value=0L
   for($b=0;$b -lt 4;$b++){
    $offset=$anchor[0]+$axis*4+$b-0x06054000
    $disc.Position=(258333+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
    $byte=$disc.ReadByte();if($byte -lt 0){throw 'Short disc read'}
    $value=($value-shl 8)-bor $byte
   }
   if($value -ge 0x80000000L){$value-=0x100000000L};$point+=$value
  }
  if($point[0] -ne 0x201000 -or $point[1] -ne 0x13000 -or $point[2] -ne $anchor[1]){throw 'Original arena anchor differs'}
 }
} finally {$disc.Dispose()}
'PASS generated Charge reads original signed arena anchors; native coordinates verified directly, no device movement acceptance'
