param([string]$DiscImage='',[string]$ScratchRoot='../streamed-pcm-test')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$scratch=[IO.Path]::GetFullPath((Join-Path $repo $ScratchRoot));New-Item -ItemType Directory -Force $scratch | Out-Null
& cl /nologo /O2 /std:c++20 /EHsc "/I$repo/include" "$PSScriptRoot/streamed_pcm_test.cpp" "/Fe:$scratch/test.exe" "/Fo:$scratch/test.obj"
if($LASTEXITCODE -ne 0){throw 'PCM fixture compile failed'}
& "$scratch/test.exe" @($(if($DiscImage){$DiscImage}))
if($LASTEXITCODE -ne 0){throw 'PCM fixture failed'}
