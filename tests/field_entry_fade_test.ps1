$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot '..'
$cmake=Get-Content -Raw (Join-Path $root 'CMakeLists.txt')
$audio=Get-Content -Raw (Join-Path $root 'src/integration/lagi_audio_vita.cpp')
if ($audio -notmatch '(?s)bool isSoundLoadingFinished\(\).*?return g_loadingFinished.load') {throw 'Audio completion contract changed'}
foreach($token in @('fieldSubTaskStatus = 0; // LAGI_FIELD_ENTRY_ZERO_STATE','if (isSoundLoadingFinished()) // LAGI_FIELD_ENTRY_AUDIO_READY')) {
    if (!$cmake.Contains($token)) {throw "Missing gate: $token"}
}
foreach($filesReady in @($false,$true)) {
    foreach($audioReady in @($false,$true)) {
        $state=0; $requests=0
        for($frame=0;$frame -lt 60;$frame++) {
            if($state -eq 0 -and $filesReady -and $audioReady) {$requests++;$state++}
        }
        $expected=if($filesReady -and $audioReady){1}else{0}
        if($requests -ne $expected) {throw 'Entry fade wait/one-shot model failed'}
    }
}
Write-Output 'PASS: Vita audio-ready contract, state-zero source guards and one-shot wait model; not native runtime/hardware validation'
