#pragma once
#include "lagi/lagi_azel_upstream_prelude.h"
#include "kernel/vdp1AnimatedQuad.h"

// Saturn-authored Gouraud words, optionally indexed by animation frame.
// The stride is a caller contract: overlapping four-byte trail windows and
// fixed eight-byte orb blocks share the same draw service.
int lagiDrawParticleWithColorTable(sAnimatedQuad* particle, sVec3_FP* position,
    sSaturnPtr colors, unsigned frameStride);
