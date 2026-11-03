#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_sprite_text.h"
#include "3dEngine_textureCache.h"

namespace lagi::worldmap {
namespace {
struct MapSpriteTextTask : s_workAreaTemplate<MapSpriteTextTask> {
    struct Glyph {s16 x, y;u16 source;};
    const MapLabelPlacement* placement = nullptr;
    std::vector<Glyph> glyphs;
    static void Draw(MapSpriteTextTask* task) {
        if (!task->placement) return;
        auto& context = graphicEngineStatus.m14_vdp1Context[0];
        for (const auto& glyph : task->glyphs) {
            auto& command = *context.m0_currentVdp1WriteEA;
            command.m0_CMDCTRL = 0x1000;
            command.m4_CMDPMOD = 0x0080;
            command.m6_CMDCOLR = 0x27C0;
            command.m8_CMDSRCA = glyph.source;
            command.mA_CMDSIZE = 0x0110;
            command.mC_CMDXA = s16(u32(task->placement->x) + u32(glyph.x));
            command.mE_CMDYA = s16(0u - u32(task->placement->y) - u32(glyph.y));
            context.m20_pCurrentVdp1Packet->m4_bucketTypes = u16(MTH_Mul(
                task->placement->nativeZ,
                graphicEngineStatus.m405C.m38_oneOverFarClip).asS32());
            context.m20_pCurrentVdp1Packet->m6_vdp1EA = &command;
            ++context.m20_pCurrentVdp1Packet;
            ++context.m1C;
            ++context.m0_currentVdp1WriteEA;
            ++context.mC;
        }
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition = {nullptr,nullptr,Draw,nullptr};
        return &definition;
    }
};
}

s_workArea* create_map_sprite_text(s_workArea* parent, sSaturnPtr string,
                                  const MapLabelPlacement* placement)
{
    if (!parent || string.isNull() || !placement) return nullptr;
    const int columns = computeStringLength(string,40);
    if (columns < 0 || columns > 40) return nullptr;
    auto* task = createSubTask<MapSpriteTextTask>(parent);
    if (!task) return nullptr;
    task->placement = placement;
    task->glyphs.reserve(columns);

    // MAIN 060263D4 saves these six context fields, prints into the authored
    // offscreen row, extracts both 8x8 tiles, clears it, then restores fields.
    const auto saved = vdp2StringContext;
    setupVDP2StringRendering(0,32,columns,2);
    VDP2DrawString(readSaturnString(string).c_str());
    constexpr u32 firstCharacter = 0x7B600;
    u32 character = firstCharacter;
    for (int column=0;column<columns;++column) {
        const auto cell = getVdp2VramU16(0x7000+column*2) & 0xFFF;
        if (!cell) continue;
        const auto below = getVdp2VramU16(0x7080+column*2) & 0xFFF;
        auto* destination = getVdp1Pointer(0x25C00000+character);
        addToMemoryLayout(destination,0x40);
        memcpy_dma(getVdp2Vram(cell*0x20),destination,0x20);
        memcpy_dma(getVdp2Vram(below*0x20),destination+0x20,0x20);
        task->glyphs.push_back({s16(column*8),0,u16(character>>3)});
        character += 0x40;
    }
    if (character != firstCharacter)
        invalidateVdp1TextureRange(firstCharacter,character-firstCharacter);
    clearVdp2TextArea();
    vdp2StringContext.m4_cursorX = saved.m4_cursorX;
    vdp2StringContext.m8_cursorY = saved.m8_cursorY;
    vdp2StringContext.mC_X = saved.mC_X;
    vdp2StringContext.m10_Y = saved.m10_Y;
    vdp2StringContext.m14_Width = saved.m14_Width;
    vdp2StringContext.m18_Height = saved.m18_Height;
    return task;
}
}
