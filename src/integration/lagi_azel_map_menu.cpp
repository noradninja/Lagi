#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"
#include "field.h"
#include "items.h"

bool lagiAzelCurrentFieldMapOwned()
{
    // Native 06012554: COMMON field->item table, not a universal B2 flag.
    auto* field=getFieldTaskPtr();
    if (!field || !gCommonFile) return false;
    const int index=field->m2C_currentFieldIndex;
    if(index<0 || index>=23) return false;
    const auto item=static_cast<eItems>(readSaturnS16(
        gCommonFile->getSaturnPtr(0x20A650)+index*2));
    return static_cast<int>(item)>=0 && mainGameState.getItemCount(item)!=0;
}

bool lagiAzelMapCellExplored(int subfield,unsigned subfieldCount,int x,int y)
{
    // Native 0601249C: current-field COMMON hierarchy and getBit566 mapping.
    auto* field=getFieldTaskPtr();
    if(!field || !gCommonFile || subfield<0 || unsigned(subfield)>=subfieldCount) return false;
    const int index=field->m2C_currentFieldIndex;
    if(index<0 || index>=23) return false;
    auto table=readSaturnEA(gCommonFile->getSaturnPtr(0x20A388)+index*4);
    if(!table.m_offset) return false;
    auto record=readSaturnEA(table+subfield*4);
    if(!record.m_offset) return false;
    auto rectangle=readSaturnEA(record);
    if(!rectangle.m_offset) return false;
    const int dx=x-readSaturnS8(rectangle),dy=y-readSaturnS8(rectangle+1);
    const int width=readSaturnS8(rectangle+2),height=readSaturnS8(rectangle+3);
    if(dx<0 || dy<0 || dx>=width || dy>=height) return false;
    const int flag=dy*width+readSaturnS16(record+4)+dx;
    if(flag<0) return false;
    return mainGameState.getBit566(static_cast<u32>(flag));
}

void lagiAzelRestoreMapViewPriorities()
{
    // Native child Delete 0601763C..5A, before the parent selection returns.
    auto* regs=vdp2Controls.m4_pendingVdp2Regs;
    if (!regs) return;
    regs->mF8_PRINA=0x0304;
    regs->mFA_PRINB=0x0700;
    regs->mFC_PRIR=0;
    vdp2Controls.m_isDirty=1;
}

void lagiAzelSetupMapViewWindow(std::int16_t scrollX,std::int16_t scrollY)
{
    // Native 060173AA..06017406: signed map scroll, window0 endpoints,
    // initial NBG2 ratio, and map-view priorities.
    auto* regs=vdp2Controls.m4_pendingVdp2Regs;
    if (!regs) return;
    const int x=0x60-int(scrollX), y=0x20-int(scrollY);
    regs->m10A_CCRNB=(regs->m10A_CCRNB&0xFFE0)|0x1F;
    regs->mC0_WPSX0=static_cast<u16>(x*2);
    regs->mC2_WPSY0=static_cast<u16>(y);
    regs->mC4_WPEX0=static_cast<u16>((x+0xC0)*2-1);
    regs->mC6_WPEY0=static_cast<u16>(y+0xBF);
    regs->mF8_PRINA=0x0304;
    regs->mFA_PRINB=0x0705;
    regs->mFC_PRIR=0;
    vdp2Controls.m_isDirty=1;
}

bool lagiAzelSetupMapEntry(bool standalone)
{
    if (!gCommonFile || !gCommonFile->m_data) return false;
    // Native 06016B04. Normal status-menu entry retains its animated scroll;
    // standalone entry resets the four layers and both native fade channels.
    if (standalone) {
        setVdp2LayerScroll(0,0,0);
        setVdp2LayerScroll(1,0,0);
        setVdp2LayerScroll(2,0,0);
        setVdp2LayerScroll(3,0,0x100);
        resetVdp2LayersAutoScroll();
        setupVDP2StringRendering(0,0x22,0x2C,0x1C);
        clearVdp2TextArea();
        fadePalette(&g_fadeControls.m0_fade0,0xC210,0xC210,1);
        fadePalette(&g_fadeControls.m24_fade1,0xC210,0xC210,1);
    }
    unpackGraphicsToVDP2(gCommonFile->m_data+0xE3D4,getVdp2Vram(0x71000));
    return true;
}

bool lagiAzelLoadFieldMapResources(const LagiAzelMapDescriptor& descriptor,
    std::uint8_t* patterns, std::size_t capacity, std::uint32_t& graphicsVramOffset)
{
    graphicsVramOffset = 0;
    if (!descriptor.mapFile || !descriptor.patternFile || !patterns) return false;
    const std::uint32_t graphicsBytes = getFileSize(descriptor.mapFile);
    const std::uint32_t patternBytes = getFileSize(descriptor.patternFile);
    // Native 06016E84: strictly greater than 4000 uses the lower destination.
    const std::uint32_t destination = graphicsBytes > 0x4000 ? 0x69000 : 0x6C000;
    if (!graphicsBytes || !patternBytes || graphicsBytes > 0x80000 - destination ||
        patternBytes > capacity || (patternBytes <= 0x800 && capacity < 0x800)) return false;
    if (loadFile2(descriptor.mapFile,getVdp2Vram(destination),0) < 0) return false;
    // 06016EEE: for a short/equal 800-byte PNB, initialize exactly 800 bytes
    // with big-endian 0238 pattern words before the native file overwrites it.
    if (patternBytes <= 0x800)
        for (unsigned i = 0; i < 0x800; i += 2) {
            patterns[i] = 0x02;
            patterns[i + 1] = 0x38;
        }
    if (loadFile2(descriptor.patternFile,patterns,0) < 0) return false;
    graphicsVramOffset = destination;
    return true;
}
