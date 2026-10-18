param([string]$ScratchRoot='../e011-scheduled-effects-test',[string]$DiscImage='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/src/integration" "$PSScriptRoot/e011_scheduled_effects_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'E011 scheduled effects test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'E011 scheduled effects test failed'}
if($DiscImage){
 $stream=[IO.File]::OpenRead($DiscImage)
 try {
  # Read original code/literals in place. No game asset is extracted or stored.
  $anchors=@{
   0x06058986=0x480B;0x06058988=0x7420;0x060589BA=0x2BB8;
   0x06058AE4=0x0602CF2A;0x06058AE8=0x0602CF38;
   0x06058A80=0x490B;0x06058A82=0x740C;0x06058ABA=0x2CC8;
   0x06058C4C=0x0601FF14;0x06058AF8=0x060312EE;
   0x06058C30=0x06035C48;0x06058C34=0x06035D4C;
   0x0605F300=0x0002;0x0605F314=0x0001;
   0x0605F5EC=0x0004;0x0605F5EE=0x010C;0x0605F5F0=0x00FC;0x0605F5F2=0x0001
  }
  foreach($address in $anchors.Keys){
   $offset=$address-0x06054000
   $stream.Position=([long]218450+[int][Math]::Floor($offset/2048))*2352+16+($offset%2048)
   $width=2;if($anchors[$address] -gt 65535){$width=4}
   $actual=0L;for($i=0;$i -lt $width;$i++){$byte=$stream.ReadByte();if($byte -lt 0){throw 'Short disc read'};$actual=($actual-shl 8)-bor $byte}
   if($actual -ne $anchors[$address]){throw ('Original E011 scheduled-effect anchor changed at {0:X8}' -f $address)}
  }
 } finally {$stream.Dispose()}
 Write-Output 'PASS original E011 quad/model task service, clock-loop and descriptor anchors; not cinematic playback.'
}
