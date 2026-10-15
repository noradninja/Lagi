#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"

namespace {
const char* mapMessageString(std::uint32_t address)
{
    if(!gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000 ||
        address<gCommonFile->m_base || address-gCommonFile->m_base>=gCommonFile->m_dataSize)
        return nullptr;
    const auto offset=address-gCommonFile->m_base;
    const auto* text=gCommonFile->m_data+offset;
    if(!std::memchr(text,0,gCommonFile->m_dataSize-offset)) return nullptr;
    return reinterpret_cast<const char*>(text);
}

void mapMessage(const char* text)
{
    // MAIN 06016B6C. 06025104 is setActiveFont, not a palette operation.
    setupVDP2StringRendering(8,0x38,0x1C,4);
    clearVdp2TextArea();
    setActiveFont(graphicEngineStatus.m40AC.fontIndex);
    if(text) {
        vdp2StringContext.m0=0;
        drawObjectName(text);
    }
}
} // namespace

void lagiAzelClearMapMessage()
{
    mapMessage(nullptr);
}

bool lagiAzelPresentMapMessage(int selected,const std::array<std::int32_t,2>& availability)
{
    if(selected==0) {
        const char* location=mapMessageString(0x20D9B9);
        const char* missing=availability[0]==1 ? nullptr : mapMessageString(0x20D9CF);
        if(!location || (availability[0]!=1 && !missing)) {
            lagiAzelClearMapMessage();return false;
        }
        mapMessage(location);
        // Native 06016BD0..BEA repositions even when no second line is needed.
        vdp2StringContext.m4_cursorX=vdp2StringContext.mC_X;
        vdp2StringContext.m8_cursorY=vdp2StringContext.m10_Y+2;
        if(missing) drawObjectName(missing);
        return true;
    }
    if(selected==1) {
        const char* text=mapMessageString(availability[1]==1 ? 0x20D9DE : 0x20D9EE);
        if(!text) {lagiAzelClearMapMessage();return false;}
        mapMessage(text);return true;
    }
    lagiAzelClearMapMessage();
    return false;
}
