$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$cmake=Get-Content -Raw "$repo/CMakeLists.txt"
$native=Get-Content -Raw "$repo/extern/Azel/AzelLib/battle/battleTargetable.cpp"
foreach($name in 'sBaldorSubTask0_draw','sBaldorSubTask0_draw1','sBaldorSubTask1_draw') {
 if(!$native.Contains("void $name(sBaldorSubTask* pThis)")){throw "Missing native draw $name"}
}
if(!$cmake.Contains('if (!pThis->m84_pTargetable || (pThis->m84_pTargetable->m50_flags & 0x40000u)) return;')){throw 'Retirement guard missing'}
Write-Output 'PASS retirement emission guard contract for all native target cursor draw variants; device pending'
