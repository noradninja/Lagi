param([string]$Build='build-battle-dev',[string]$ScratchRoot='../epk-actor-scope-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
foreach($entry in @(@('town_townCutscene.cpp','sCutsceneCommandDefaultTask','// 0600ee56','epk_draw.inc'),@('town_e011_twn_e011.cpp','sE011Task1','// 06059ac8','attached_draw.inc'))){
 $source=Get-Content -Raw "$repo/$Build/azel_upstream/$($entry[0])"
 $begin=$source.IndexOf("    static void Draw($($entry[1])* pThis)")
 $end=$source.IndexOf($entry[2],$begin)
 if($begin -lt 0 -or $end -le $begin){throw 'Generated actor Draw boundary changed'}
 [IO.File]::WriteAllText("$scratch/$($entry[3])",$source.Substring($begin,$end-$begin))
}
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/epk_actor_scope_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'EPK actor scope fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'EPK actor scope fixture failed'}
