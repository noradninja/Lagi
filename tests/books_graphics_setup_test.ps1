param([string]$ScratchRoot='..\books-setup-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_books_menu.cpp')
$begin=$source.IndexOf('bool lagiAzelBooksSetupGraphics(')
$end=$source.IndexOf('bool lagiAzelBooksPresentText(',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Native graphics setup source boundary missing'}
Set-Content -LiteralPath (Join-Path $scratch 'books_setup_production.inc') -Value $source.Substring($begin,$end-$begin)
$exe=Join-Path $scratch 'books_setup_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'books_graphics_setup_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Books graphics setup host compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Books graphics setup host execution failed'}
