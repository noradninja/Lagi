param([string]$Build='build-battle-dev',[string]$ScratchRoot='../waterfall-position-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/$Build/azel_upstream/field_field_a3_a3_waterfall.cpp"
if(!$source.Contains('pTask->m_UpdateMethod = &s_A3_WaterfallLarge::UpdateType2;') -or
 !$source.Contains('lagiWaterfallSmallVariantEmit(pThis);') -or
 !$source.Contains('lagiWaterfallLargeEmit(pThis, alternate);')){throw 'Native waterfall variant routing missing'}
if($source.Contains('(u8*)pThis +')){throw 'Waterfall helpers retain raw task offset reads'}
$begin=$source.IndexOf('static const s32 waterfallVelocityScaleTable0')
$end=$source.IndexOf('static const s16 waterfallModelOffsets0',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Waterfall helper fixture boundary changed'}
[IO.File]::WriteAllText("$scratch/waterfall_helpers.inc",$source.Substring($begin,$end-$begin))
& cl /nologo /O2 /EHsc /std:c++20 "/I$scratch" "/I$repo/src/integration" "$PSScriptRoot/waterfall_position_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Waterfall helper fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Waterfall helper fixture failed'}
