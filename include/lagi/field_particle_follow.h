#pragma once
#include <memory>
struct sParticlePoolManager;struct sParticleSpawnConfig;struct sVec3_FP;
// The aliasing shared pointer keeps the position's storage alive until the
// particle finishes, even if its producing actor has already retired.
int lagiSpawnFieldParticleFollowingXZ(sParticlePoolManager*,const sParticleSpawnConfig&,
                                    const std::shared_ptr<sVec3_FP>&,int animationCycles);
