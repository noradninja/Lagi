param([string]$ScratchRoot='../tutorial-a7-dialog-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/field/field_a7/a7_dialogChoiceTask.cpp"
$start=$source.IndexOf('// 0x10-byte sibling task')
if($start-lt 0){throw 'Native A7 dialogue boundary changed'}
[IO.File]::WriteAllText("$scratch/native_a7_dialog.inc",$source.Substring($start))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/tutorial_a7_dialog_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE-ne 0){throw 'Native A7 dialogue compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE-ne 0){throw 'Native A7 dialogue assertions failed'}
