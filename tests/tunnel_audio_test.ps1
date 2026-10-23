param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function ReadWord([long]$ea,[int]$bytes,[long]$lba,[long]$base){
  $value=0L;for($i=0;$i -lt $bytes;$i++){
   $offset=$ea+$i-$base;$disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+$offset%2048
   $value=($value-shl 8)-bor $disc.ReadByte()
  };return $value
 }
 foreach($anchor in @(@(0x060547A8,0x0602C32A,4),@(0x06054730,0xE464,2),@(0x06054736,0xE465,2))){
  if((ReadWord $anchor[0] $anchor[2] 244544 0x06054000) -ne $anchor[1]){throw 'Original tunnel stop call contract differs'}
 }
 $words=@(0xE700,0xD346,0x6673,0x432B,0xE502)
 for($i=0;$i -lt $words.Count;$i++){
  if((ReadWord (0x0602C32A+$i*2) 2 58 0x06006000) -ne $words[$i]){throw 'Original soundFunc command contract differs'}
 }
 if((ReadWord 0x0602C448 4 58 0x06006000) -ne 0x0602B188){throw 'Original stop enqueue target differs'}
 'PASS original tunnel call sites and MAIN shared stop wrapper'
}finally{$disc.Dispose()}
$source=Get-Content -Raw "$repo/$Build/azel_upstream/dummy.cpp"
$wrapper=[regex]::Match($source,'void soundFunc\(int sound\) \{[^}]+\}')
if(!$wrapper.Success){throw 'Generated stop wrapper absent'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-tunnel-audio-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/sound_stop.inc",$wrapper.Value)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/tunnel_audio_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Tunnel audio fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Tunnel audio fixture failed'}
