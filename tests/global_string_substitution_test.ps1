param([string]$GeneratedSource='..\navigator-text-test\azel_upstream\VDP2.cpp',[string]$ScratchRoot='..\global-string-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw ([IO.Path]::GetFullPath((Join-Path $repo $GeneratedSource)))
$functions=@()
foreach($name in @('printVdp2StringSub2','printVdp2String')){
 $match=[regex]::Match($source,"void $name\([^;]*?\)\s*\{")
 if(!$match.Success){throw 'Generated parser function missing'}
 $begin=$match.Index;$end=$begin+$match.Length;$depth=1
 while($depth -gt 0 -and $end -lt $source.Length){if($source[$end] -eq '{'){$depth++};if($source[$end] -eq '}'){$depth--};$end++}
 if($depth){throw 'Generated parser boundary missing'}
 $functions+=$source.Substring($begin,$end-$begin)
}
Set-Content -LiteralPath (Join-Path $scratch 'global_string_production.inc') -Value ($functions -join "`n")
$exe=Join-Path $scratch 'global_string_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'global_string_substitution_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Actual parser host compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Actual parser execution failed'}
