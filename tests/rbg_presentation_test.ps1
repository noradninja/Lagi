$ErrorActionPreference='Stop'
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-rbg-presentation-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$shader=Get-Content -Raw "$PSScriptRoot/../shaders/vdp2_rbg0_f.cg"
$helper=[regex]::Match($shader,'(?s)float rbgProjectionX\(.*?\n\}')
if(!$helper.Success){throw 'Production RBG projection helper missing'}
Set-Content -LiteralPath "$scratch/rbg_projection_shader.inc" -Value $helper.Value
& cl /nologo /O2 /std:c++20 /EHsc "/I$scratch" "$PSScriptRoot/rbg_presentation_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'RBG presentation fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'RBG presentation fixture failed'}
