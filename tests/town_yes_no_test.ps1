param([Parameter(Mandatory=$true)][string]$Build,
      [Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead((Resolve-Path -LiteralPath $DiscImage).Path)
try {
    function Read-Native([long]$lba,[long]$base,[long]$address,[int]$count) {
        $bytes=New-Object byte[] $count
        for($i=0;$i -lt $count;$i++) {
            $offset=$address+$i-$base
            $disc.Position=($lba+[long][Math]::Floor($offset/2048))*2352+16+($offset%2048)
            $byte=$disc.ReadByte()
            if($byte -lt 0) { throw 'Short native read' }
            $bytes[$i]=$byte
        }
        return [BitConverter]::ToString($bytes)
    }
    if((Read-Native 182 0x00200000 0x0021006C 16) -ne '00-00-00-00-06-01-C4-F0-06-01-C5-D8-06-01-C6-B0' -or
       (Read-Native 58 0x06006000 0x0600D09E 2) -ne '06-14' -or
       (Read-Native 58 0x06006000 0x0600D760 4) -ne '06-01-C6-CA' -or
       (Read-Native 58 0x06006000 0x0601C558 4) -ne '00-21-00-48' -or
       (Read-Native 58 0x06006000 0x0601C6EC 16) -ne 'E0-18-81-4A-E0-14-81-4B-E0-10-81-4D-E0-04-81-4E') {
        throw 'Native Yes/No opcode/task/text/layout binding differs'
    }
} finally { $disc.Dispose() }
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/town_townScript.cpp"
$start=$generated.IndexOf('        case 38: // native MAIN0600D660')
$end=$generated.IndexOf('        case 39: // multi-choice',$start)
if($start -lt 0 -or $end -lt $start) { throw 'Native Yes/No opcode missing' }
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-town-yes-no-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/opcode.inc",$generated.Substring($start,$end-$start))
$source=Get-Content -Raw "$repo/src/integration/lagi_town_yes_no_vita.cpp"
[IO.File]::WriteAllText("$scratch/production.inc",[regex]::Replace($source,'(?m)^#include[^\r\n]*',''))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/extern/Azel/AzelLib" "$PSScriptRoot/town_yes_no_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE) { throw 'Town Yes/No fixture compile failed' }
& "$scratch/test.exe"
if($LASTEXITCODE) { throw 'Town Yes/No fixture failed' }
