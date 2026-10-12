param([string]$ScratchRoot='..\map-particles-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
$source=Get-Content -Raw (Join-Path $scratch 'azel_upstream/field_fieldItemBox_type2.cpp')
$gates=[regex]::Matches($source,'if \([^\r\n]+\) // LAGI_MAP_PARTICLE_GATE')
if($gates.Count -ne 2){throw 'Particle Update/Draw gates missing'}
$body="bool updateBlocked(Parent* pParent){$($gates[0].Value)`nreturn true; return false;}`n"
$body+="bool drawBlocked(Parent* pParent){$($gates[1].Value)`nreturn true; return false;}"
Set-Content -LiteralPath (Join-Path $scratch 'map_particle_gates.inc') -Value $body
$exe=Join-Path $scratch 'map_particle_gate_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$scratch" (Join-Path $PSScriptRoot 'map_particle_gate_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Particle gate host compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Particle gate host execution failed'}
