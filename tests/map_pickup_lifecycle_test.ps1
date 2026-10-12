param([string]$GeneratedSource='..\map-pickup-test\azel_upstream\field_fieldItemBox_type1.cpp',[string]$ScratchRoot='..\map-pickup-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw ([IO.Path]::GetFullPath((Join-Path $repo $GeneratedSource)))
$functions=@()
foreach($name in @('LCSItemBox_Callback1','LCSItemBox_UpdateType1')){
 $match=[regex]::Match($source,"void $name\([^;]*?\)\s*\{")
 if(!$match.Success){throw 'Generated map function missing'}
 $begin=$match.Index;$end=$begin+$match.Length;$depth=1
 while($depth -gt 0 -and $end -lt $source.Length){if($source[$end] -eq '{'){$depth++};if($source[$end] -eq '}'){$depth--};$end++}
 if($depth){throw 'Generated map function boundary missing'}
 $functions+=$source.Substring($begin,$end-$begin)
}
Set-Content -LiteralPath (Join-Path $scratch 'map_pickup_production.inc') -Value ($functions -join "`n")
$exe=Join-Path $scratch 'map_pickup_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'map_pickup_lifecycle_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Actual map host compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Actual map execution failed'}
