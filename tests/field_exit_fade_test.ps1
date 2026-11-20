param([string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$source=Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/field.cpp"
$a=$source.IndexOf('struct exitCutsceneTaskUpdateSub0Sub1Sub0 :')
$b=$source.IndexOf('void exitCutsceneTaskUpdateSub0Sub1(s32 fieldIndex',$a)
if($a -lt 0 -or $b -le $a){throw 'Generated shared exit task boundary changed'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-field-fade-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
[IO.File]::WriteAllText("$scratch/field_exit_task.inc",$source.Substring($a,$b-$a))
& cl /nologo /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/field_exit_fade_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Field fade fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Field fade fixture failed'}
