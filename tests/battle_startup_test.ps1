param([string]$ScratchRoot='../battle-startup-test',[string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$cache=Get-Content -Raw "$repo/$BattleBuild/CMakeCache.txt"
if($cache -notmatch 'LAGI_BATTLE_EXECUTE_A3:BOOL=ON'){throw 'Configure the opt-in A3 execution build first'}
$manager=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/kernel_moduleManager.cpp"
if(!$manager.Contains('loadBattle,         // 4: native story battle, COMMON selects encounter')){throw 'Story battles are still disabled in the module manager'}
$boss=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_BTL_A3_2_BTL_A3_2.cpp"
if(!$boss.Contains('createBattleMainTask') -or !$boss.Contains('0x60a84bc')){throw 'Native Arachnoth engine entry missing'}
$overlay=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_battleOverlay.cpp"
if($overlay.IndexOf('if (!lagiPrepareNativeA3Battle()) return;') -gt $overlay.IndexOf('loadFnt("ITEM.FNT")') -or !$overlay.Contains('if (!lagiPrepareNativeA3Battle()) return;')){throw 'Battle preflight must precede font allocation'}
if(!$overlay.Contains('if (!lagiEnterNativeA3Battle(pThis)) return;') -or !$overlay.Contains('if (pThis->m2_numLoadedFnt == 0)')){throw 'A3 factory retry/font ownership guard absent'}
$drawBegin=$overlay.IndexOf('static void battleOverlayTask_Draw(');$drawEnd=$overlay.IndexOf('static void battleOverlayTask_Delete(',$drawBegin)
if($drawBegin -lt 0 -or $drawEnd -le $drawBegin){throw 'Native overlay Draw boundary changed'}
[IO.File]::WriteAllText("$scratch/overlay.inc",$overlay.Substring($drawBegin,$drawEnd-$drawBegin))
if($overlay -match 'overlayStart_BTL_\w+\(pThis\)'){throw 'Unintegrated overlay startup escaped the native execution boundary'}
$generic=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/battle_battleGenericData.cpp"
if(!$generic.Contains('if (!m_data || m_dataSize < lagi::battle_generic_minimum_bytes) return;')){throw 'Missing-resource guard absent before native generic table parsing'}
$source=Get-Content -Raw "$repo/src/integration/lagi_battle_startup_vita.cpp"
$begin=$source.IndexOf('namespace {')
if($begin -lt 0){throw 'Production startup service boundary changed'}
[IO.File]::WriteAllText("$scratch/service.inc",$source.Substring($begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/include" "$PSScriptRoot/battle_startup_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Battle startup test compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Battle startup test failed'}
