param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Native([long]$ea,[int]$count){
  $v=0L
  for($i=0;$i -lt $count;++$i){
   $o=$ea+$i-0x6054000
   $disc.Position=(218450+[long][math]::Floor($o/2048))*2352+16+($o%2048)
   $b=$disc.ReadByte();if($b -lt 0){throw 'Short disc read'}
   $v=($v-shl 8)-bor $b
  };return $v
 }
 if((Native 0x605401C 2) -ne 0xD231 -or
    (Native 0x6054020 2) -ne 0x420B -or
    (Native 0x6054022 2) -ne 0xE443 -or
    (Native 0x60540E4 4) -ne 0x0602B400){throw 'Original E011 loadSoundBanks(67,0) anchors differ'}
}finally{$disc.Dispose()}
$file=Join-Path $repo "$Build/azel_upstream/town_e011_twn_e011.cpp"
if(!(Test-Path $file)){throw "Generated E011 source missing: $file"}
$source=Get-Content $file -Raw
if($source -notmatch 'loadSoundBanks\(67, 0\); // Native E011' -or
    $source -match 'loadSoundBanks\(68, 0\)'){throw 'Generated E011 did not retain native bank 67'}
Write-Output 'PASS original-disc E011 call/argument and generated music bank 67; not listening validation'
