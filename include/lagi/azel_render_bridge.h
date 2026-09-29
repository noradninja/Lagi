#pragma once

#include <cstdint>

struct sProcessed3dModel;

namespace lagi::azel_bridge {

// Reset per-frame submission diagnostics before Azel task draw execution.
void begin_frame();

// Number of model submissions observed through Azel's native render boundary
// during the current frame.
std::uint32_t submission_count();

// Most recently submitted Azel processed model. This remains opaque at the
// bridge layer until the live-model adapter converts it to Vdp1ModelSource.
sProcessed3dModel* last_model();

} // namespace lagi::azel_bridge
