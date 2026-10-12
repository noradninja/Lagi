param([string]$ScratchRoot = '..\books-presentation-test')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$scratch = [IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source = Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_books_menu.cpp')
$begin = $source.IndexOf('bool lagiAzelBooksPresentText(')
$end = $source.IndexOf('bool lagiAzelBooksPresentWindow(', $begin)
if ($begin -lt 0 -or $end -le $begin) { throw 'Production Books service boundary missing' }
Set-Content -LiteralPath (Join-Path $scratch 'books_presentation_production.inc') -Value $source.Substring($begin,$end-$begin)
$exe = Join-Path $scratch 'books_presentation_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'books_presentation_test.cpp') (Join-Path $repo 'src/integration/lagi_azel_books_resource.cpp') "/Fe:$exe" "/Fo:$scratch/"
if ($LASTEXITCODE -ne 0) { throw 'Books presentation host compilation failed' }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Books presentation execution failed: $LASTEXITCODE" }
