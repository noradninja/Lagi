param([string]$Build='build-battle-dev',[string]$Disc='D:/Users/Noradninja/Desktop/Panzer Dragoon Saga Disc 1 of 4 (U)(Saturn)/AZEL_1.bin')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-native-colors-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/native_color_animation_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Native color fixture compilation failed'}
& "$scratch/test.exe" $Disc
if($LASTEXITCODE -ne 0){throw 'Native color fixture failed'}
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/native_color_draw_test.cpp" "/Fe:$scratch/draw.exe" "/Fo:$scratch/draw.obj"
if($LASTEXITCODE -ne 0){throw 'Color hierarchy fixture compilation failed'}
& "$scratch/draw.exe"
if($LASTEXITCODE -ne 0){throw 'Color hierarchy fixture failed'}
$source=Get-Content -Raw "$repo/$Build/azel_upstream/mainMenuDebugTasks.cpp"
foreach($variant in @(0,2,5,7,9,11)){
 $expected="void modelDrawFunction$variant(s_3dModel* pModel)`n{`n    lagiDrawColorAnimatedModel(pModel);`n}"
 if(!$source.Replace("`r`n","`n").Contains($expected)){throw "Colored draw variant $variant missing"}
}
$service=(Get-Content -Raw "$repo/src/integration/lagi_native_color_draw.inc").Replace("`r`n","`n").Trim()
if(!$source.Replace("`r`n","`n").Contains($service)){throw 'Generated native color draw helper is stale'}
'PASS all six generated colored draw variants share the native hierarchy/color path'
