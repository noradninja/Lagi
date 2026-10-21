param([string]$ScratchRoot='../battle-life-meter-test',[string]$Build='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cmake "-DREPO=$repo" "-DSCRATCH=$scratch" -P "$PSScriptRoot/battle_life_meter_fixture.cmake"
if($LASTEXITCODE -ne 0){throw 'Shared life-meter source adapter failed'}
$sources=@('battle_BTL_A3_urchin.cpp','battle_BTL_A3_baldorQueen.cpp','battle_BTL_A5_BTL_A5_single.cpp','battle_BTL_A5_BTL_A5_variant.cpp')
$statements=@()
foreach($name in $sources){
 $text=Get-Content -Raw "$scratch/$name"
 $sites=[regex]::Matches($text,'static_cast<sEnemyLifeMeterTask\*>\((?:lifeMeter|pThis->mC8|pThis->m40_enemyLifeMeterTask|pThis->mAC_lifeMeter)\)->m31 \|= ([128]);')
 foreach($site in $sites){$statements += $site.Value}
 if($name -like 'battle_BTL_A3_*'){
  $live=Get-Content -Raw "$repo/$Build/azel_upstream/$name"
  if($live.Contains('[0x31]')){throw "Live source retains raw meter writes: $name"}
  foreach($site in $sites){if(!$live.Contains($site.Value)){throw "Live typed flag statement missing: $name"}}
 }
}
if($statements.Count -ne 12){throw 'Expected all 12 known raw meter writes to be adapted'}
$cases=@();$masks=@();for($i=0;$i -lt $statements.Count;$i++){
 $cases += "case ${i}: $($statements[$i]) break;"
 $masks += [regex]::Match($statements[$i],'\|= ([128])').Groups[1].Value
}
# Mechanical extraction of production statements and the unchanged native update.
[IO.File]::WriteAllText("$scratch/signals.inc","constexpr unsigned masks[]={$($masks -join ',')};`nvoid signal(sEnemyLifeMeterTask* meter,unsigned index){p_workArea lifeMeter=meter;Actor actor{meter,meter,meter};auto* pThis=&actor;switch(index){$($cases -join "`n") default:assert(false);}}")
$native=Get-Content -Raw "$repo/$Build/azel_upstream/battle_battleEnemyLifeMeter.cpp"
$begin=$native.IndexOf('void createEnemyLifeMeterTask_update(')
$end=$native.IndexOf('const std::array<u16, 10> enemyLifeGaugeColors', $begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native life-meter update boundary changed'}
[IO.File]::WriteAllText("$scratch/update.inc",$native.Substring($begin,$end-$begin))
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "/I$repo/extern/Azel/AzelLib" "$PSScriptRoot/battle_life_meter_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Life-meter lifecycle fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Life-meter lifecycle fixture failed'}
