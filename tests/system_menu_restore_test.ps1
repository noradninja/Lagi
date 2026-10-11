$ErrorActionPreference = 'Stop'
$source = Get-Content -Raw (Join-Path $PSScriptRoot '../src/integration/lagi_system_menu_vita.cpp')
foreach ($task in @('Save','Load')) {
    if ($source -notmatch ('self->childMode = ChildMode::' + $task + ';\s*self->preservePresentation\(\);\s*self->child = create' + $task + 'Task\(self\);')) {
        throw "$task must preserve presentation before synchronous child Init"
    }
}
if ($source -notmatch 'self->childMode = ChildMode::None;\s*self->restorePresentation\(\);\s*render\(self\);') { throw 'Restore must precede System redraw' }
if ($source -notmatch '\*vdp2Controls.m4_pendingVdp2Regs = savedRegisters;' -or
    $source -match '(?<!\*)vdp2Controls.m4_pendingVdp2Regs = ') { throw 'Restore values, not the pending-buffer pointer' }
foreach ($pair in @(
    @('savedRegisters = \*vdp2Controls.m4_pendingVdp2Regs;', '\*vdp2Controls.m4_pendingVdp2Regs = savedRegisters;'),
    @('savedLayers\[index\] = graphicEngineStatus.m40BC_layersConfig\[index\];', 'graphicEngineStatus.m40BC_layersConfig\[index\] = savedLayers\[index\];'),
    @('savedFade = g_fadeControls;', 'g_fadeControls = savedFade;'),
    @('savedFont = pVdp2StringControl->m0_index;', 'setActiveFont\(savedFont\);')
)) {
    if ($source -notmatch $pair[0] -or $source -notmatch $pair[1]) { throw "Missing round-trip field: $($pair[0])" }
}
$load = $source.IndexOf('lagiAzelSaveMarkInGameLoad();')
$restore = $source.IndexOf('self->restorePresentation();')
if ($load -lt 0 -or $load -gt $restore -or $source.Substring($load,$restore-$load) -notmatch 'return;') { throw 'Successful Load must transition without returning to System' }
Write-Output 'PASS: System presentation round-trip fields, child-init ordering, current-buffer restoration and successful-load bypass (source guards; not hardware)'
