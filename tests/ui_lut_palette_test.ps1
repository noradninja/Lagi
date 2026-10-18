param([string]$ScratchRoot='../ui-lut-palette-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch|Out-Null
& cl /nologo /O2 /EHsc /std:c++20 "/I$repo/include" "$PSScriptRoot/ui_lut_palette_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'UI LUT fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'UI LUT fixture failed'}
$source=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
$start=$source.IndexOf('static GpuMode1Texture* findOrUploadVdp1UiTexture(')
$capture=$source.IndexOf('lagi::capture_ui_lut_palette(',$start)
$hit=$source.IndexOf('entry.hasPalette && entry.palette==palette',$start)
$refresh=$source.IndexOf('if (refreshEntry) {',$start)
$allocate=$source.IndexOf('entry.gpu.data = probeGpuAlloc(',$start)
if($capture -le $start -or $hit -le $capture -or $refresh -le $hit -or $allocate -le $refresh){throw 'Palette validation or allocation-free refresh order changed'}
Write-Output 'PASS: production cache validates colors before hit and refreshes before allocation.'
