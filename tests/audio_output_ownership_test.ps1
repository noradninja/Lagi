$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=Join-Path ([IO.Path]::GetTempPath()) ('lagi-audio-owner-'+[guid]::NewGuid())
New-Item -ItemType Directory $scratch | Out-Null
$source=Get-Content -Raw "$repo/src/integration/lagi_audio_vita.cpp"
$parts=@()
foreach($signature in 'void worker_fade_all()','s32 findSound(s32 soundIndex)') {
 $start=$source.IndexOf($signature);if($start -lt 0){throw "Missing $signature"}
 $open=$source.IndexOf('{',$start);$depth=1;$end=$open+1
 while($depth -gt 0){if($source[$end] -eq '{'){$depth++};if($source[$end] -eq '}'){$depth--};$end++}
 $parts+=$source.Substring($start,$end-$start)
}
[IO.File]::WriteAllText("$scratch/production.inc",($parts -join "`n"))
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "/I$scratch" "$PSScriptRoot/audio_output_ownership_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'Audio ownership compile failed'}
& "$scratch/test.exe"
if($LASTEXITCODE -ne 0){throw 'Audio ownership fixture failed'}
