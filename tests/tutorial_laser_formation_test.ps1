param([string]$ScratchRoot='../tutorial-laser-formation-test')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $root $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$root/include" "$PSScriptRoot/tutorial_laser_formation_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Tutorial formation fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Tutorial formation fixture failed'}
