$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-registration-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_render_bridge.cpp"
$a=$source.IndexOf('void register_native_model_resource(')
$b=$source.IndexOf('const std::vector<RegisteredModelResource>& published_model_resources()', $a)
if($a -lt 0 -or $b -le $a){throw 'Registration boundary changed'}
[IO.File]::WriteAllText("$scratch/registration.inc",$source.Substring($a,$b-$a))
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/model_registration_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Registration fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Registration fixture failed'}
