#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"
#include "field.h"
#include "items.h"
#include "kernel/menuSprite.h"

bool lagiAzelCurrentFieldMapOwned()
{
    // Native 06012554: COMMON field->item table, not a universal B2 flag.
    auto* field=getFieldTaskPtr();
    if (!field || !gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000 ||
        gCommonFile->m_dataSize<0xA650+23*2) return false;
    const int index=field->m2C_currentFieldIndex;
    if(index<0 || index>=23) return false;
    const auto item=static_cast<eItems>(readSaturnS16(
        gCommonFile->getSaturnPtr(0x20A650)+index*2));
    return static_cast<int>(item)>=0 && mainGameState.getItemCount(item)!=0;
}

static bool mapCommonRange(const auto& pointer,std::size_t length)
{
    return gCommonFile && pointer.m_offset>=gCommonFile->m_base &&
        std::size_t(pointer.m_offset-gCommonFile->m_base)<=gCommonFile->m_dataSize &&
        length<=gCommonFile->m_dataSize-std::size_t(pointer.m_offset-gCommonFile->m_base);
}

static bool mapCellExploredSlot(int subfield,int x,int y)
{
    // Native 0601249C: current-field COMMON hierarchy and getBit566 mapping.
    auto* field=getFieldTaskPtr();
    if(!field || !gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000 ||
        gCommonFile->m_dataSize<0xA388+23*4 || subfield<0 ||
        std::uint64_t(subfield)*4+4>gCommonFile->m_dataSize) return false;
    const int index=field->m2C_currentFieldIndex;
    if(index<0 || index>=23) return false;
    auto table=readSaturnEA(gCommonFile->getSaturnPtr(0x20A388)+index*4);
    if(!mapCommonRange(table,4)) return false;
    const auto slot=table+static_cast<u32>(subfield)*4u;
    if(!mapCommonRange(slot,4)) return false;
    auto record=readSaturnEA(slot);
    if(!mapCommonRange(record,10)) return false;
    auto rectangle=readSaturnEA(record);
    if(!mapCommonRange(rectangle,4)) return false;
    const int dx=x-readSaturnS8(rectangle),dy=y-readSaturnS8(rectangle+1);
    const int width=readSaturnS8(rectangle+2),height=readSaturnS8(rectangle+3);
    if(dx<0 || dy<0 || dx>=width || dy>=height) return false;
    // MOV.W displacement4 at 060124EA is byte offset8 (word scaled).
    // Record+4 is the marker-list pointer, not the exploration bit base.
    const int flag=dy*width+readSaturnS16(record+8)+dx;
    if(flag<0) return false;
    return mainGameState.getBit566(static_cast<u32>(flag));
}

bool lagiAzelMapCellExplored(int subfield,unsigned subfieldCount,int x,int y)
{
    return subfield>=0 && unsigned(subfield)<subfieldCount && mapCellExploredSlot(subfield,x,y);
}

static bool presentMapPatternsSlot(const std::uint8_t* first,std::size_t firstBytes,
    const std::uint8_t* second,std::size_t secondBytes,int subfield)
{
    if(!first || !second || firstBytes<0x800 || secondBytes<0x800 ||
        subfield<0 || !getFieldTaskPtr() || !gCommonFile) return false;
    struct Context {int subfield;} context{subfield};
    const auto explored=[](void* opaque,unsigned x,unsigned y) {
        const auto& state=*static_cast<const Context*>(opaque);
        return mapCellExploredSlot(state.subfield,int(x),int(y));
    };
    const bool owned=lagiAzelCurrentFieldMapOwned();
    // Native 06017376..88: separate authored source halves, native planes.
    if(!lagiAzelConvertMapPatterns(first,firstBytes,getVdp2Vram(0x70000),0x800,
        owned,15,explored,&context)) return false;
    return lagiAzelConvertMapPatterns(second,secondBytes,getVdp2Vram(0x70800),0x800,
        owned,31,explored,&context);
}

bool lagiAzelPresentMapPatterns(const std::uint8_t* first,std::size_t firstBytes,
    const std::uint8_t* second,std::size_t secondBytes,int subfield,unsigned subfieldCount)
{
    return subfield>=0 && unsigned(subfield)<subfieldCount &&
        presentMapPatternsSlot(first,firstBytes,second,secondBytes,subfield);
}

bool lagiAzelMapContextIsCurrent(const LagiAzelMapContext& context)
{
    LagiAzelMapContext live;
    return lagiAzelResolveCurrentMapContext(live) && live.field==context.field &&
        live.subfield==context.subfield && live.descriptor.nativeAddress==context.descriptor.nativeAddress;
}

bool lagiAzelPresentMapContextPatterns(const LagiAzelMapContext& context,
    const std::uint8_t* first,std::size_t firstBytes,const std::uint8_t* second,std::size_t secondBytes)
{
    return lagiAzelMapContextIsCurrent(context) &&
        presentMapPatternsSlot(first,firstBytes,second,secondBytes,context.subfield);
}

static unsigned drawMapObjectMarkersSlot(int subfield,
    std::int16_t scrollX,std::int16_t scrollY,std::int16_t verticalOffset)
{
    auto* field=getFieldTaskPtr();
    if(!field || !gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000 ||
        gCommonFile->m_dataSize<0xA388+23*4 || subfield<0 ||
        std::uint64_t(subfield)*4+4>gCommonFile->m_dataSize) return 0;
    const int index=field->m2C_currentFieldIndex;
    if(index<0 || index>=23) return 0;
    auto table=readSaturnEA(gCommonFile->getSaturnPtr(0x20A388)+index*4);
    if(!mapCommonRange(table,4)) return 0;
    const auto slot=table+static_cast<u32>(subfield)*4u;
    if(!mapCommonRange(slot,4)) return 0;
    auto record=readSaturnEA(slot);
    if(!mapCommonRange(record,8)) return 0;
    auto marker=readSaturnEA(record+4);
    // Original MAIN 06043260/68. No invented sprite for reserved types2/3.
    static const s_menuSprite sprites[2]={{0x23F4,0x0108,-4,-4},{0x23F0,0x0108,-4,-4}};
    unsigned emitted=0;
    while(marker.m_offset>=gCommonFile->m_base &&
        marker.m_offset-gCommonFile->m_base<=gCommonFile->m_dataSize &&
        gCommonFile->m_dataSize-(marker.m_offset-gCommonFile->m_base)>=16) {
        const unsigned type=readSaturnU8(marker+0xE);
        if(type&4) break;
        const int flag=readSaturnS16(marker+0xC);
        std::array<std::int16_t,2> screen;
        if((type&3)<2 && flag>=0 && mainGameState.getBit566(static_cast<u32>(flag)) &&
            lagiAzelMapMarkerScreenPosition(intDivide(0x10000,readSaturnS32(marker)),
                intDivide(0x10000,readSaturnS32(marker+8))+0x400,
                scrollX,scrollY,verticalOffset,screen)) {
            drawMenuSprite(&sprites[type&3],screen[0],screen[1],0x760);
            ++emitted;
        }
        marker=marker+16;
    }
    return emitted;
}

unsigned lagiAzelDrawMapObjectMarkers(int subfield,unsigned subfieldCount,
    std::int16_t scrollX,std::int16_t scrollY,std::int16_t verticalOffset)
{
    return subfield>=0 && unsigned(subfield)<subfieldCount
        ? drawMapObjectMarkersSlot(subfield,scrollX,scrollY,verticalOffset) : 0;
}

unsigned lagiAzelDrawMapContextObjectMarkers(const LagiAzelMapContext& context,
    std::int16_t scrollX,std::int16_t scrollY,std::int16_t verticalOffset)
{
    return lagiAzelMapContextIsCurrent(context)
        ? drawMapObjectMarkersSlot(context.subfield,scrollX,scrollY,verticalOffset) : 0;
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

std::array<std::int16_t,2> lagiAzelSetMapViewCenter(std::int16_t x,std::int16_t y)
{
    const auto& backing=graphicEngineStatus.m40BC_layersConfig[1];
    const auto origin=lagiAzelMapViewScrollOrigin(x,y,
        static_cast<s16>(backing.scrollX),static_cast<s16>(backing.scrollY));
    auto& map=graphicEngineStatus.m40BC_layersConfig[2];
    map.scrollX=origin[0];
    map.scrollY=origin[1];
    return origin;
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
        patternBytes > capacity || (patternBytes <= 0x800 && capacity < 0x1000)) return false;
    if (loadFile2(descriptor.mapFile,getVdp2Vram(destination),0) < 0) return false;
    // 06016EEE fills SECOND half 0024C5A4 for short/equal 800-byte PNB.
    // 06016EF8 loads the file at FIRST half 0024BDA4. Do not overwrite the
    // first-half padding or leave the second half uninitialized.
    if (patternBytes <= 0x800)
        for (unsigned i = 0x800; i < 0x1000; i += 2) {
            patterns[i] = 0x02;
            patterns[i + 1] = 0x38;
        }
    if (loadFile2(descriptor.patternFile,patterns,0) < 0) return false;
    graphicsVramOffset = destination;
    return true;
}
