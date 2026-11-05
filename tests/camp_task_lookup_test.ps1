param([Parameter(Mandatory=$true)][string]$Build,
      [Parameter(Mandatory=$true)][string]$DiscImage)
$ErrorActionPreference='Stop'
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
    # Original recovery resolves the native task through MAIN0600CE0A and
    # writes its +11 substate. MAIN indexes the single native +70 task array.
    if((Read-Native 239428 0x06054000 0x060565C4 4) -ne '06-00-CE-0A' -or
       (Read-Native 239428 0x06054000 0x0605649A 10) -ne 'E2-0A-63-F2-E0-11-03-24-7F-04' -or
       (Read-Native 58 0x06006000 0x0600CE1C 14) -ne '60-43-40-08-63-53-73-70-00-3E-00-0B-00-09') {
        throw 'Original Camp recovery/task lookup binding differs'
    }
} finally { $disc.Dispose() }
$repo=Split-Path $PSScriptRoot -Parent
$generated=Join-Path $repo "$Build/azel_upstream"
$script=Get-Content -Raw "$generated/town_townScript.cpp"
$camp=Get-Content -Raw "$generated/town_camp_twn_camp.cpp"
if(([regex]::Matches($camp,'getNpcDataByIndexAsTask\(')).Count -ne 7) {
    throw 'All seven Camp dragon script lookups must use their registered task'
}
$production=''
foreach($item in @(
    @($script,'p_workArea getNpcDataByIndexAsTask'),
    @($camp,'int recoverAllHP_BP'),
    @($camp,'int scriptFunction_6056484')
)) {
    $body=[regex]::Match($item[0],'(?ms)^'+[regex]::Escape($item[1])+'\([^\n]*?\).*?^}')
    if(!$body.Success) { throw "Production function missing: $($item[1])" }
    $production+=$body.Value+"`n"
}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-camp-task-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",$production)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/camp_task_lookup_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE) { throw 'Camp task lookup fixture compile failed' }
& "$scratch/test.exe"
if($LASTEXITCODE) { throw 'Camp task lookup fixture failed' }
