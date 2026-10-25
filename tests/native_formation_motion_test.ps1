param([string]$ScratchRoot='../native-formation-motion-test')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $root $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "/I$root/include" "$PSScriptRoot/native_formation_motion_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Native formation motion fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Native formation motion fixture failed'}
$compiler='E:/dev/VitaSDK/bin/arm-vita-eabi-g++.exe'
& $compiler -std=c++20 -fsyntax-only "-I$root/include" "$PSScriptRoot/native_formation_motion_test.cpp"
if($LASTEXITCODE-ne 0){throw 'Vita formation motion syntax failed'}
