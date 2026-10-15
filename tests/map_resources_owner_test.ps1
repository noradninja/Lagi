param([string]$ScratchRoot='..\map-resources-owner-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_map_resources_vita.cpp')
$begin=$source.IndexOf('LagiAzelMapResources::~LagiAzelMapResources()')
if($begin -lt 0){throw 'Production Map resource owner missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_owner_production.inc') -Value $source.Substring($begin)
$source=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_azel_map_menu.cpp')
$begin=$source.IndexOf('bool lagiAzelLoadFieldMapResources(')
if($begin -lt 0){throw 'Production native Map loader missing'}
Set-Content -LiteralPath (Join-Path $scratch 'map_owner_loader_production.inc') -Value $source.Substring($begin)
$parent=Get-Content -Raw (Join-Path $repo 'src/integration/lagi_map_menu_vita.cpp')
$restore=$parent.IndexOf('self->resources.restore();')
if(!$parent.Contains('self->resources.load(self->context.descriptor)') -or
 $restore -lt 0 -or $restore -gt $parent.LastIndexOf('initVdp2ForStatusMenu();')) {
 throw 'Map parent resource load/restore ordering missing'
}
$exe=Join-Path $scratch 'map_resources_owner_test.exe'
& cl /nologo /std:c++20 /EHsc "/I$repo/include" "/I$scratch" (Join-Path $PSScriptRoot 'map_resources_owner_test.cpp') "/Fe:$exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Map ownership host compile failed'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Map ownership host execution failed'}
