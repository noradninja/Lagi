param([string]$Disc='D:/Users/Noradninja/Desktop/Panzer Dragoon Saga Disc 1 of 4 (U)(Saturn)/042 Panzer Dragoon Saga Disc 1 of 4 (U).bin',[string]$ScratchRoot='../frontend-font-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source=Get-Content -Raw "$repo/extern/Azel/AzelLib/VDP2.cpp"
$begin=$source.IndexOf('void unpackGraphicsToVDP2(');$end=$source.IndexOf('void loadFont()',$begin)
if($begin -lt 0 -or $end -le $begin){throw 'Upstream unpacker boundary changed'}
$palette=[regex]::Match($source,'(?s)u8 defaultFontPalettes\[16 \* 2 \* 16\] = \{.*?\};')
if(!$palette.Success){throw 'Upstream palette boundary changed'}
[IO.File]::WriteAllText("$scratch/native_font.inc",$source.Substring($begin,$end-$begin)+"`n"+$palette.Value)
$renderer=Get-Content -Raw "$repo/src/platform/vita/neptune_renderer.cpp"
foreach($part in @(
 @('static int measureFrontendText(','static void drawCharSmallToBuffer(','frontend_text.inc'),
 @('static void launcherPixel(','static void launcherRing(','frontend_rect.inc'),
 @('static void launcherBlit(int asset','void launcher_fade(','frontend_launcher.inc'))) {
 $begin=$renderer.IndexOf($part[0]);$end=$renderer.IndexOf($part[1],$begin)
 if($begin -lt 0 -or $end -le $begin){throw 'Frontend production composition boundary changed'}
 [IO.File]::WriteAllText("$scratch/$($part[2])",$renderer.Substring($begin,$end-$begin))
}
if(([regex]::Matches($renderer,'drawFrontendLoading\(\);')).Count -ne 2){throw 'Both startup Loading paths must use the same font helper'}
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/frontend_font_test.cpp" "$repo/src/frontend_font.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/"
if($LASTEXITCODE -ne 0){throw 'Frontend font host compilation failed'}
& "$scratch/test.exe" $Disc "$scratch/font-preview.ppm" "$repo/assets/frontend"
if($LASTEXITCODE -ne 0){throw 'Frontend font assertions failed'}
