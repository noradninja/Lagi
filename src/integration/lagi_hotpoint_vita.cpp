#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"

std::vector<s_hotpointDefinition>* sHotpointBundle::getData(s32 numBones)
{
    if (m_EA.isNull())
        return nullptr;

    if (!m_cachedData.empty()) {
        if (static_cast<s32>(m_cachedData.size()) != numBones)
            return nullptr;
        return &m_cachedData;
    }

    if (numBones <= 0 || numBones > 512)
        return nullptr;

    for (s32 i = 0; i < numBones; ++i) {
        s_hotpointDefinition def{};
        sSaturnPtr entry = m_EA + static_cast<int>(8 * i);
        sSaturnPtr data = readSaturnEA(entry);
        def.m4_count = readSaturnU32(entry + 4);

        if (def.m4_count > 1024)
            return nullptr;

        for (u32 j = 0; j < def.m4_count; ++j) {
            s_hotpoinEntry hp{};
            hp.m0 = readSaturnS32(data + 0);
            hp.m4 = readSaturnVec3(data + 4);
            hp.m10 = readSaturnS32(data + 0x10);
            data += 0x14;
            def.m0.push_back(hp);
        }

        m_cachedData.push_back(def);
    }

    return &m_cachedData;
}
