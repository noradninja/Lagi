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
       (Read-Native 58 0x06006000 0x0600CE1C 14) -ne '60-43-40-08-63-53-73-70-00-3E-00-0B-00-09' -or
       (Read-Native 239428 0x06054000 0x06055492 8) -ne '51-D8-D3-14-41-08-01-E6' -or
       (Read-Native 239428 0x06054000 0x0607BAC8 4) -ne '00-00-00-09' -or
       (Read-Native 239428 0x06054000 0x060580AC 4) -ne '00-00-00-09' -or
       (Read-Native 239428 0x06054000 0x060564BE 24) -ne 'E0-11-02-4C-E3-0A-32-33-89-02-4F-26-00-0B-E0-01-E0-00-4F-26-00-0B-00-09' -or
       (Read-Native 239428 0x06054000 0x06056414 8) -ne 'E2-0C-E0-11-00-0B-04-24' -or
       (Read-Native 239428 0x06054000 0x06056426 24) -ne 'E0-11-02-4C-E3-0C-32-33-89-02-4F-26-00-0B-E0-01-E0-00-4F-26-00-0B-00-09' -or
       (Read-Native 239428 0x06054000 0x060565CC 20) -ne '06-04-A6-5B-06-04-2F-14-06-06-D7-02-06-04-A6-5A-06-04-2F-1B' -or
       (Read-Native 58 0x06006000 0x06042F14 1) -ne '80' -or
       (Read-Native 58 0x06006000 0x06042F1B 1) -ne '01' -or
       (Read-Native 239428 0x06054000 0x06056530 20) -ne 'E0-11-E3-09-02-4C-32-33-89-02-E1-07-E0-11-04-14-AF-C9-4F-26' -or
       (Read-Native 239428 0x06054000 0x06071AA8 12) -ne '06-00-CE-0A-06-07-FA-84-06-08-01-5C' -or
       (Read-Native 239428 0x06054000 0x0607092A 8) -ne '84-4C-CB-08-00-0B-80-4C' -or
       (Read-Native 239428 0x06054000 0x0607093E 8) -ne '84-4C-C9-F7-00-0B-80-4C' -or
       (Read-Native 239428 0x06054000 0x06070950 4) -ne '84-4C-C8-06' -or
       (Read-Native 239428 0x06054000 0x0606D7E8 8) -ne '06-03-61-F4-06-03-55-50' -or
       (Read-Native 239428 0x06054000 0x0606C9B4 4) -ne '06-06-AE-CC' -or
       (Read-Native 239428 0x06054000 0x06070578 18) -ne '90-35-61-CE-0E-D4-21-18-70-FF-0E-D4-70-02-8D-1E-0E-D4' -or
       (Read-Native 239428 0x06054000 0x06074000 4) -ne '06-06-04-23' -or
       (Read-Native 239428 0x06054000 0x0606048C 4) -ne '06-02-6D-A4' -or
       (Read-Native 239428 0x06054000 0x0606D416 8) -ne '4F-22-D4-29-7F-FC-D2-2C' -or
       (Read-Native 239428 0x06054000 0x0606D542 8) -ne '2F-E6-4F-22-D5-18-DE-0F' -or
       (Read-Native 239428 0x06054000 0x0606D580 4) -ne 'AF-84-00-09' -or
       (Read-Native 58 0x06006000 0x06026DA4 24) -ne 'D4-3C-E2-08-D1-3A-D3-3C-60-30-80-48-E0-00-80-49-80-4A-80-4B-00-0B-21-20') {
        throw 'Original Camp recovery/task lookup binding differs'
    }
} finally { $disc.Dispose() }
$repo=Split-Path $PSScriptRoot -Parent
$generated=Join-Path $repo "$Build/azel_upstream"
$script=Get-Content -Raw "$generated/town_townScript.cpp"
$camp=Get-Content -Raw "$generated/town_camp_twn_camp.cpp"
$dragon=Get-Content -Raw "$generated/town_camp_campDragon.cpp"
if(!$camp.Contains('for (int i = 0; i < 9; i++)')) {
    throw 'Camp scripts can select nine authored setups'
}
foreach($binding in @(
    'm_oneArg[0x06056408] = {&lagiCampStartFeeding,',
    'm_oneArg[0x0605641C] = {&lagiCampFeedingFinished,',
    'm_zeroArg[0x060564D6] = {&lagiCampRewardPetting,',
    'm_zeroArg[0x060564FE] = {&lagiCampRewardInteraction,',
    'm_oneArg[0x06056526] = {&lagiCampFinishInteraction,',
    'm_zeroArg[0x06071A78] = {&lagiCampUseScriptedPlayerAnimations,',
    'm_zeroArg[0x06071A90] = {&lagiCampUseNormalPlayerAnimations,',
    'm_oneArg[0x0607091E] = {&lagiCampDisableLookAtDecay,',
    'm_oneArg[0x06070932] = {&lagiCampEnableLookAtDecay,',
    'm_oneArg[0x06070946] = {&lagiCampNpcMotionFinished,',
    'm_threeArg[0x06070198] = {&lagiCampTurnNpcTowardsNpc,',
    'm_twoArg[0x06070268] = {&lagiCampTurnNpcBack,',
    'm_threeArg[0x06070560] = {&lagiCampSetNpcAnimation,',
    'm_threeArg[0x06070664] = {&lagiCampScheduleNpcAnimation,',
    'm_oneArg[0x0606D790] = {&lagiCampRandomChoice,',
    'm_zeroArg[0x0606AECC] = {&setupCameraUpdateForCurrentMode,',
    'm_zeroArg[0x0606CC0E] = {&setupAutoWalk,',
    'm_zeroArg[0x0606D416] = {&townIsCutsceneDone,',
    'm_zeroArg[0x0606D580] = {&townDeleteCutscene,',
    'm_oneArg[0x0606D542] = {&townCreateCutscene,'
)) {
    if(!$camp.Contains($binding)) { throw "Camp shared service missing: $binding" }
}
if(!$script.Contains('{0x06026DA4, {&lagiTownOpenSaveMenu, "town save menu"}},')) {
    throw 'Authored town save station callback is unbound'
}
if(([regex]::Matches($camp,'getNpcDataByIndexAsTask\(')).Count -ne 13) {
    throw 'Camp dragon script and recovery-wait lookups must use their registered task'
}
if(!$camp.Contains('overlayScriptFunctions.m_oneArg[0x060564B4] = {&lagiCampRecoveryFinished, "Camp recovery finished"};')) {
    throw 'Native Camp recovery wait is unbound'
}
$production=''
foreach($item in @(
    @($script,'p_workArea getNpcDataByIndexAsTask'),
    @($script,'sNPC* getNpcDataByIndex'),
    @($script,'static s32 lagiTownOpenSaveMenu'),
    @($camp,'int recoverAllHP_BP'),
    @($camp,'int scriptFunction_6056484'),
    @($camp,'static s32 lagiCampRecoveryFinished'),
    @($camp,'static s32 lagiCampStartFeeding'),
    @($camp,'static s32 lagiCampFeedingFinished'),
    @($camp,'static s32 lagiCampRewardPetting'),
    @($camp,'static s32 lagiCampRewardInteraction'),
    @($camp,'static s32 lagiCampFinishInteraction'),
    @($camp,'static s32 lagiCampUseScriptedPlayerAnimations'),
    @($camp,'static s32 lagiCampUseNormalPlayerAnimations'),
    @($camp,'static s32 lagiCampDisableLookAtDecay'),
    @($camp,'static s32 lagiCampEnableLookAtDecay'),
    @($camp,'static s32 lagiCampNpcMotionFinished'),
    @($camp,'static s32 lagiCampRandomChoice'),
    @($camp,'static s32 lagiCampSetNpcAnimation'),
    @($camp,'static s32 lagiCampTurnNpcTowardsNpc'),
    @($camp,'static s32 lagiCampTurnNpcBack'),
    @($camp,'static s32 lagiCampScheduleNpcAnimation'),
    @($dragon,'s32 sCampDragon_InitSub1')
)) {
    $body=[regex]::Match($item[0],'(?ms)^'+[regex]::Escape($item[1])+'\([^\n]*?\).*?^}')
    if(!$body.Success) { throw "Production function missing: $($item[1])" }
    $production+=$body.Value+"`n"
}
$registration=[regex]::Match($dragon,'(?m)^\s*npcData0\.m70_npcPointerArray\[readSaturnS32\(arg \+ 0x20\)\]\.workArea = pThis;')
if(!$registration.Success) { throw 'Camp dragon must register the native long index' }
$production+='void registerProductionDragon(sTownDragon* pThis, sSaturnPtr arg) {'+"`n"+$registration.Value+"`n}"+"`n"
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-camp-task-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/production.inc",$production)
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/camp_task_lookup_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE) { throw 'Camp task lookup fixture compile failed' }
& "$scratch/test.exe"
if($LASTEXITCODE) { throw 'Camp task lookup fixture failed' }
