param([Parameter(Mandatory=$true)][string]$DiscImage,[string]$BuildDirectory='build-battle-dev')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$disc=[IO.File]::OpenRead($DiscImage)
try {
 function Read-Native([long]$ea,[int]$size){$v=0L;for($i=0;$i -lt $size;$i++){$o=$ea+$i-0x06054000;$disc.Position=(258144+[long][Math]::Floor($o/2048))*2352+16+($o%2048);$v=($v-shl 8)-bor $disc.ReadByte()};return $v}
 foreach($p in @(@(0x0606FDA8,4,0x060ABAB0),@(0x0606FDAC,4,0x0606E9D2),@(0x0606FCB2,2,0x420B),@(0x060ABAB0,4,18),@(0x060ABAB4,4,21),@(0x060ABABC,4,0),@(0x0606E8CE,2,0xE610),@(0x0606E8CA,2,0xE702))){if((Read-Native $p[0] $p[1]) -ne $p[2]){throw 'Native result clear contract differs'}}
}finally{$disc.Dispose()}
$generated=Get-Content -Raw "$repo/$BuildDirectory/azel_upstream/battle_battleResultScreen.cpp"
if(!$generated.Contains('lagiBattleResultClearArea(g_BTL_GenericData->getSaturnPtr(0x060ABAB0))') -or $generated.Contains('battleResultScreen_createProgressiveText2(pThis, g_BTL_GenericData->getSaturnPtr(0x060ABAB0))')){throw 'Result coordinate record still dispatched as text'}
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-result-clear-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/battle_result_clear_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Result clear fixture compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Result clear fixture failed'}
