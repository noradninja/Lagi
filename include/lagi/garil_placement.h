#pragma once
// Include after the Azel prelude.
struct sFieldCameraStatus;
struct s_dragonTaskWorkArea;
// Exposes the existing native altitude/configuration producer, without springs.
void lagiAzelPrepareGarilCamera(sFieldCameraStatus*,s_dragonTaskWorkArea*);
bool lagiAzelTryGarilPlacement(sSaturnPtr);
