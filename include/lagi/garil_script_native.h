#pragma once
// Include after the Azel prelude. False preserves the existing foreign-overlay
// dispatch; only recovered A5 actions with the actual A5 owner are intercepted.
bool lagiAzelTryGarilScriptAction(sSaturnPtr, s32 argument, s32& result);
