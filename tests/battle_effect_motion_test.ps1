param([string]$ScratchRoot='../battle-effect-motion-test',[string]$DiscImage='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/battle_effect_motion_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Effect motion test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Effect motion test failed'}
if($DiscImage){
 $disc=[IO.File]::OpenRead($DiscImage)
 try {
  # Read original BTL_A3 instructions/literals directly; no extracted game
  # code or data is written into the repository or test artifacts.
  $anchors=@{
   0x0608CB18=0x2102;0x0608CB64=0x2102;0x0608CB92=0x2102;
   0x0608CBC8=0x2102;0x0608CC20=0x2102;0x0608CC4C=0x2102;
   0x0608CD58=0x00AA;0x0608CD5A=0xAAAA;
   0x0608CD60=0x0400;0x0608CD62=0;
   0x0608CAB8=0x1000;0x0608CABA=0;
   0x0608D738=0xFD71;0x0608D73A=0x0CCC;
   0x0608D61E=0x600D;0x0608D620=0x8800;
   0x0608D622=0x897D;0x0608D624=0x8801;
   0x0608D706=0x1E16;0x0608D70E=0x1E27;0x0608D718=0x1E18;
   0x0608CEC0=0x028F;0x0608D464=0x03D7;
   0x0608D328=0x0001;0x0608D32A=0x2345;
   0x0608D214=0xC81F;0x0608D46C=0x0FFF;0x0608D46E=0xFFFF;
   0x0608D5CA=0x75FF;0x0608D5E2=0xE532;
   0x0608D5F4=0x600B;0x0608D5F6=0x70D8;0x0608D736=0x00A4;
   0x06099D06=0xE500;0x06099DE0=0x051E;
   0x06099D56=0xC802;0x06099DE2=0x3334;0x06099DE4=0x0CCC;
   0x06099D70=0x3308;0x06099D72=0x1E3B
  }
  foreach($address in $anchors.Keys){
   $offset=[long]$address-0x06054000
   $physical=([long]258144+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $null=$disc.Seek($physical,[IO.SeekOrigin]::Begin)
   $word=($disc.ReadByte() -shl 8) -bor $disc.ReadByte()
   if($word -ne $anchors[$address]){throw ('Original effect anchor mismatch at {0:X8}' -f $address)}
  }
  Write-Output 'PASS: original BTL_A3 motion stores and speed/angle/magnitude literals match.'
 } finally {$disc.Dispose()}
}
