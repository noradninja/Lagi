param([Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$f=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$ea,[int]$size){$v=0L;for($i=0;$i -lt $size;$i++){$o=$ea+$i-0x06054000;$f.Position=(258333+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$v=($v-shl 8)-bor $f.ReadByte()};return $v}
 foreach($p in @(@(0x0606D7A6,2,0xE704),@(0x0606D7AA,2,0xE626),@(0x0606D7AE,2,0xE519),@(0x0606D7C2,2,0xE403),@(0x0606D8D0,4,0x060251AC),@(0x0606D8D8,4,0x06025352),@(0x0606D9D8,2,0x8539),@(0x060A910C,4,16),@(0x060A9114,4,60))){if((Read-Native $p[0] $p[1]) -ne $p[2]){throw 'Native speech contract differs'}}
} finally {$f.Dispose()}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-subtitle-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",(Get-Content -Raw "$repo/src/integration/lagi_battle_subtitle.inc"))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/battle_subtitle_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Subtitle fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Subtitle fixture failed'}
