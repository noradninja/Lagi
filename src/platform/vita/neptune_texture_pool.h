#pragma once

#include <cstddef>

namespace lagi::platform::renderer {

// Slabs never move their mapped payloads. Return an existing tail that can
// hold the aligned allocation, or size() when pool growth is necessary.
template<class Slabs>
std::size_t findTextureSlabWithSpace(const Slabs& slabs, std::size_t bytes)
{
    for (std::size_t i = 0; i < slabs.size(); ++i) {
        const auto& slab = slabs[i];
        if (slab.used <= slab.bytes && bytes <= slab.bytes - slab.used)
            return i;
    }
    return slabs.size();
}

} // namespace lagi::platform::renderer
