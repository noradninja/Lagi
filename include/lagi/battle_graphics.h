#pragma once

// Platform-owned storage for the native 0602849A/06028668 graphics handoff.
// Resident video-file bytes are staged before native audio cleanup begins.
void* lagiCaptureBattleGraphics(bool includeFade);
bool lagiPrepareBattleGraphicsRestore(void* backup);
bool lagiRestoreBattleGraphics(void*& backup);
void lagiDiscardBattleGraphics(void*& backup);
