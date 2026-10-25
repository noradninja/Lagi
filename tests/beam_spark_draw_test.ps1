param([string]$BattleBuild='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-spark-draw-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
function Get-SourceBlock([string]$source,[string]$first,[string]$next){
    $start=$source.IndexOf($first);$end=$source.IndexOf($next,$start)
    if($start -lt 0 -or $end -le $start){throw "Native math/source boundary changed: $first"}
    return $source.Substring($start,$end-$start)
}
$source=Get-Content -Raw "$repo/$BattleBuild/azel_upstream/field_fieldBeamSpark.cpp"
if(!$source.Contains('#include "lagi_field_beam_spark_draw.inc"') -or $source.Contains('Unimplemented();')){throw 'Native spark draw callbacks not integrated'}
$start=$source.IndexOf('// Beam spark effect')
[IO.File]::WriteAllText((Join-Path $scratch 'spark_service.inc'),$source.Substring($start))
[IO.File]::WriteAllText((Join-Path $scratch 'spark_header.inc'),(Get-Content -Raw "$repo/extern/Azel/AzelLib/field/fieldBeamSpark.h").Replace('#pragma once',''))
$generated=Get-Content -Raw "$repo/extern/Azel/AzelLib/common_generated.cpp"
$table=[regex]::Match($generated,'s32 CosSinTable\[5120\] = \{[\s\S]*?\};').Value
if(!$table){throw 'Native trig table boundary changed'}
$trigo=Get-Content -Raw "$repo/extern/Azel/AzelLib/trigo.cpp"
$math=$table+"`n"+(Get-SourceBlock $trigo 'fixedPoint getCos(u32 value)' 'fixedPoint atan_FP(')
$fixed=Get-Content -Raw "$repo/extern/Azel/AzelLib/fixedPoint.cpp"
$math+="`n"+(Get-SourceBlock $fixed 'fixedPoint MTH_Mul(fixedPoint a, fixedPoint b)' 'sVec3_FP MTH_Mul(')
$engine=Get-Content -Raw "$repo/extern/Azel/AzelLib/3dEngine.cpp"
foreach($bounds in @(@('void initMatrixToIdentity(','void updateEngineCamera('),@('void rotateMatrixX(','void rotateCurrentMatrixShiftedX('),@('void rotateMatrixY(','void rotateCurrentMatrixY('),@('void transformAndAddVec(','void transformAndAddVecByCurrentMatrix('))){
    $math+="`n"+(Get-SourceBlock $engine $bounds[0] $bounds[1])
}
[IO.File]::WriteAllText((Join-Path $scratch 'spark_native_math.inc'),$math)
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$repo/src/integration" "/I$scratch" "$PSScriptRoot/beam_spark_draw_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Beam spark draw fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Beam spark draw fixture failed'}
