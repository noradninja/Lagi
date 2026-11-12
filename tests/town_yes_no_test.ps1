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
    # The intro flag is set before the prompt. The result alone selects
    # the reply, so reading the flag cannot stand in for Yes.
    if ((Read-Native 239428 0x06054000 0x06058911 3) -ne '11-04-1A' -or
        (Read-Native 239428 0x06054000 0x06058A6C 24) -ne '0F-09-04-1A-07-00-00-09-06-06-C9-52-26-08-00-01-05-09-00-09-06-05-8C-50' -or
        (Read-Native 239428 0x06054000 0x06058A84 8) -ne '07-00-00-09-06-06-C9-5C' -or
        (Read-Native 239428 0x06054000 0x06058C50 12) -ne '07-01-00-09-06-06-D5-42-06-06-14-A5') {
        throw 'Gash lift conversation or Yes/No branch differs'
    }
    if ((Read-Native 239428 0x06054000 0x06060184 20) -ne '27-02-00-09-06-06-53-D8-06-06-53-E7-15-02-00-09-06-06-02-95' -or
        (Read-Native 239428 0x06054000 0x0606034C 8) -ne '06-09-00-09-06-05-8F-77') {
        throw 'Leave the Camp no longer dispatches the gift scene'
    }
    if ((Read-Native 239428 0x06054000 0x06058011 8) -ne '06-00-09-06-06-5F-98-01' -or
        (Read-Native 239428 0x06054000 0x06065F98 12) -ne '07-01-00-09-06-00-CC-B4-00-00-00-03' -or
        (Read-Native 239428 0x06054000 0x06060354 20) -ne '07-01-00-09-06-07-1E-20-00-00-00-1E-20-07-00-09-06-03-01-1E') {
        throw 'Camp setup 3 or departure fade and termination differ'
    }
    if ((Read-Native 239428 0x06054000 0x060591D8 12) -ne '30-09-00-3C-00-6A-00-01-2B-01-00-6A' -or
        (Read-Native 239428 0x06054000 0x0605930D 7) -ne '0F-00-C4-0F-09-00-2F') {
        throw 'Gash gift inventory or story operands differ'
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
