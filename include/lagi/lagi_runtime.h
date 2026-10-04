#pragma once

namespace lagi::azel {

// Initialize Azel through its native boot path. Lagi owns only platform,
// filesystem, input, timing and presentation services around that runtime.
bool runtime_init();

// Advance one host frame of the native Azel task graph and publish the
// resulting presentation state to Neptune.
void runtime_frame();

} // namespace lagi::azel
