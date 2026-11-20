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
    if((Read-Native 239428 0x06054000 0x0607B5BC 16) -ne '06-05-77-14-06-05-7A-68-00-00-00-00-06-05-79-F0' -or
       (Read-Native 239428 0x06054000 0x06057804 16) -ne '25-F0-02-40-06-02-FD-7A-00-05-B0-5B-06-02-17-98' -or
       (Read-Native 239428 0x06054000 0x06057B90 10) -ne '50-44-88-03-05-29-25-58-89-02') {
        throw 'Native Camp interactive task/palette/state callback anchors differ'
    }
} finally { $disc.Dispose() }
$generated=Get-Content -Raw "$repo/$Build/azel_upstream/town_camp_twn_camp.cpp"
if($generated -notmatch 'case 0x0607B5BC:' -or
   $generated -notmatch 'return lagiCreateCampInteractiveActor\(parent, arg\)' -or
   $generated -notmatch 'm_oneArg\[0x06057B62\]' -or
   $generated -notmatch 'm_oneArg\[0x06057B76\]') { throw 'Generated Camp actor bindings absent' }
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-camp-interactive-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_camp_interactive_actor.inc"
[IO.File]::WriteAllText("$scratch/production.inc",[regex]::Replace($source,'(?m)^#include[^\r\n]*',''))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "/I$repo/extern/Azel/AzelLib" "$PSScriptRoot/camp_interactive_actor_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE) { throw 'Camp interactive actor fixture compile failed' }
& "$scratch/test.exe"
if($LASTEXITCODE) { throw 'Camp interactive actor fixture failed' }
