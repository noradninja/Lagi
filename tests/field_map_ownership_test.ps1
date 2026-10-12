param([string]$AzelSourceRoot='E:/dev/Lagi/extern/Azel/AzelLib',[string]$ScratchRoot='..\field-map-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
Push-Location $scratch
try {
 & cmake "-DREPO_ROOT=$repo" "-DLAGI_AZEL_SOURCE_ROOT=$AzelSourceRoot" -P (Join-Path $PSScriptRoot 'generate_native_menu_sources.cmake')
 if($LASTEXITCODE -ne 0){throw 'Native item-box source generation failed'}
 foreach($type in 0..2){
  $path=Join-Path $scratch "azel_upstream/field_fieldItemBox_type$type.cpp"
  $source=Get-Content -Raw $path
  if($source -match 'pThis->m2[01]\b'){throw 'Unrelated duplicate item-box target flags remain'}
  & E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe -std=c++20 -fsyntax-only -DLAGI_PLATFORM_VITA=1 "-I$repo/include" "-I$AzelSourceRoot" "-I$AzelSourceRoot/field" $path
  if($LASTEXITCODE -ne 0){throw 'Native item-box Vita syntax failed'}
 }
 $source=Get-Content -Raw (Join-Path $scratch 'azel_upstream/field_fieldItemBox_type1.cpp')
 $begin=$source.IndexOf('    if ((pThis->m88_poseIdx == 0)')
 $end=$source.IndexOf('    updateLCSTarget(&pThis->m8_LCSTarget);',$begin)
 if($begin -lt 0 -or $end -le $begin){throw 'Actual map ownership tail missing'}
 $end += '    updateLCSTarget(&pThis->m8_LCSTarget);'.Length
 Set-Content -LiteralPath (Join-Path $scratch 'map_ownership_production.inc') -Value $source.Substring($begin,$end-$begin)
 $common=Get-Content -Raw (Join-Path $AzelSourceRoot 'common.h')
 $methods=@()
 foreach($name in @('setBit','clearBit','getBit','getItemCount','addItemCount')){
  $signature=if($name -in @('getItemCount','addItemCount')){"$name(eItems itemIndex"}else{"$name(u32 bitIndex)"}
  $at=$common.IndexOf($signature)
  $begin=$common.LastIndexOf("`n",$at)+1
  $brace=$common.IndexOf('{',$at);$depth=1;$end=$brace+1
  while($depth -gt 0 -and $end -lt $common.Length){if($common[$end] -eq '{'){$depth++};if($common[$end] -eq '}'){$depth--};$end++}
  if($at -lt 0 -or $depth -ne 0){throw 'Actual native ownership method missing'}
  $methods+=$common.Substring($begin,$end-$begin)
 }
 Set-Content -LiteralPath (Join-Path $scratch 'item_ownership_production.inc') -Value ($methods -join "`n")
 $exe=Join-Path $scratch 'field_map_test.exe'
 & cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'field_map_ownership_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
 if($LASTEXITCODE -ne 0){throw 'Map ownership host compile failed'}
 & $exe
 if($LASTEXITCODE -ne 0){throw 'Map ownership host execution failed'}
} finally {Pop-Location}
