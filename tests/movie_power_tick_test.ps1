$ErrorActionPreference = 'Stop'
# Lifecycle/rate model and source guards; not a Vita idle-timer test.
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/integration/lagi_azel_movie_runtime.cpp')
$update = $source.Substring($source.IndexOf('std::uint32_t lagiAzelMovieLastUpdate()'))
if ($update -notmatch '(?s)if \(!fileInfoStruct.mC_gfsHandle\).*?return 0;' -or
    $update -notmatch '(?s)if \(lagi::azel::movie_backend_active\(\)\) \{.*?sceKernelPowerTick\(SCE_KERNEL_POWER_TICK_DEFAULT\)' -or
    $source -match 'sceKernelPowerLock') { throw 'Keep-awake must be stream/backend scoped and use ticks, not locks' }
foreach ($name in @('lagiAzelMovieStreamOpen','lagiAzelMovieStreamClose')) {
    $start = $source.IndexOf("void $name(")
    $end = $source.IndexOf("`n}", $start)
    if (!$source.Substring($start,$end-$start).Contains('g_lastMoviePowerTickUs = 0;')) { throw "Missing tick reset in $name" }
}
$intervalMatch = [regex]::Match($update, 'kPowerTickIntervalUs = ([0-9]+)u')
if (!$intervalMatch.Success) { throw 'Power tick interval missing' }
$interval = [long]$intervalMatch.Groups[1].Value
$last = 0L
$ticks = 0
for ($frame = 0; $frame -lt 300; ++$frame) {
    $now = 10000000L + $frame * 33334L
    if (!$last -or $now-$last -ge $interval) { $last=$now; ++$ticks }
}
if ($ticks -ne 10 -or $interval -ne 1000000) { throw "Unexpected tick cadence: $ticks in 10 seconds" }
Write-Output 'PASS: active-stream/backend guard, open/close resets and one-second tick cadence (CPU/source checks only)'
