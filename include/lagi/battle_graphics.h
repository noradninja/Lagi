#pragma once

// Platform-owned storage for the native 0602849A/06028668 graphics handoff.
// This does not snapshot/reload resident files; that service is separate.
void* lagiCaptureBattleGraphics(bool includeFade);
void lagiRestoreBattleGraphics(void*& backup);
void lagiDiscardBattleGraphics(void*& backup);
