param([string]$ScratchRoot='../native-indicator-sort-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/native_indicator_sort_key_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native indicator key compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Native indicator key assertions failed'}
$generated=Join-Path $repo 'build-battle-dev/azel_upstream/battle_battleTargetable.cpp'
if(Test-Path $generated) {
    $source=Get-Content -Raw $generated
    if(([regex]::Matches($source,'m4_bucketTypes = lagi::nativeIndicatorSortKey')).Count -ne 9){throw 'Not all nine generated indicator packets restored'}
    if($source.Contains('m20_pCurrentVdp1Packet->m4_bucketTypes = 0;')){throw 'Indicator zero bucket remains'}
}
