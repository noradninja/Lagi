param([string]$ScratchRoot=(Join-Path $env:TEMP 'lagi-navigator-lifecycle'), [switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$null=New-Item -ItemType Directory -Force -Path $ScratchRoot
$ScratchRoot=(Resolve-Path $ScratchRoot).Path
# Mechanical test fixture: actual service bodies, with only include directives
# removed so the host fixture can supply platform/task mocks.
$source=Get-Content -Raw (Join-Path $root 'src/integration/lagi_azel_field_navigator.cpp')
$body=[regex]::Replace($source,'(?m)^#include[^\r\n]*\r?\n','')
Set-Content -LiteralPath (Join-Path $ScratchRoot 'navigator_body.inc') -Value $body -Encoding utf8
$heapSource=Get-Content -Raw (Join-Path $root 'src/integration/lagi_heap_vita.cpp')
$heapBody=[regex]::Replace($heapSource,'(?m)^#include[^\r\n]*\r?\n','')
Set-Content -LiteralPath (Join-Path $ScratchRoot 'heap_body.inc') -Value $heapBody -Encoding utf8
Push-Location $ScratchRoot
try {
 $sanitizerFlags=@()
 if ($AddressSanitizer) {$sanitizerFlags=@('/fsanitize=address','/Zi')}
 & cl /nologo /std:c++20 /EHsc @sanitizerFlags "/I$ScratchRoot" (Join-Path $PSScriptRoot 'navigator_lifecycle_test.cpp') /Fe:navigator_lifecycle_test.exe /Fo:navigator_lifecycle_test.obj
 if ($LASTEXITCODE -ne 0) {throw 'Navigator host fixture compile failed'}
 & .\navigator_lifecycle_test.exe
 if ($LASTEXITCODE -ne 0) {throw 'Navigator actual-service lifecycle test failed'}
} finally {Pop-Location}
