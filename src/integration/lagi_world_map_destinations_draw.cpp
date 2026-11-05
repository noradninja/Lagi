#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_destinations_draw.h"
#include "3dModels.h"
#include "kernel/fileBundle.h"
#include "kernel/animation.h"
#include "kernel/vdp1AnimatedQuad.h"
#include "kernel/vdp1Allocator.h"
#include "mainMenuDebugTasks.h"

// MAIN0602E5CC: original scale and sorting-only bias, shared with field pools.
int lagiDrawPoolParticle(sAnimatedQuad*,sVec3_FP*,fixedPoint,fixedPoint);

namespace lagi::worldmap {
struct MapDestinationModels {
    std::vector<sVdp1Quad> frames;
    sAnimatedQuad quad;
    s_3dModel model;
    std::array<s32,50> progress{};
    u32 flags=0x10; // camera Init06057336..42 enables the destination pass
};
static void refresh_destination_flags(MapDestinationModels& state)
{
    // WORLD06056194..0605626A; preserve every unrelated control bit.
    const auto& b=mainGameState.bitField;
    state.flags&=~15u;
    if((b[7]&2) && !(b[11]&1))state.flags|=4;
    if((b[8]&0x40) && !(b[9]&0x40))state.flags|=1;
    if((b[9]&0x40) && !(b[12]&0x20))state.flags|=2;
    if(b[8]&0x40)state.flags|=8;
}
MapDestinationModels* initialize_map_destination_models(sSaturnPtr program,s_workAreaCopy* owner)
{
    if(program.isNull() || !owner || !owner->m0_fileBundle || !owner->m4_vd1Allocation)return nullptr;
    auto* state=new MapDestinationModels;
    // Init06055E08: animated sprite06058B44, owner VRAM, scale00020000.
    state->frames=initVdp1Quad(program+0x4B44);
    if(state->frames.empty()){delete state;return nullptr;}
    particleInitSub(&state->quad,owner->m4_vd1Allocation->m4_vdp1Memory,&state->frames);
    refresh_destination_flags(*state);
    auto* bundle=owner->m0_fileBundle;
    auto* hierarchy=bundle->getModelHierarchy(0x1E8);
    auto* pose=hierarchy?bundle->getStaticPose(0x5C4,hierarchy->countNumberOfBones()):nullptr;
    // Native stack arguments: hierarchy01E8, animation0, pose05C4,
    // color-animation05CC, hotpoints0. These are MCB table slots.
    if(!pose || !init3DModelRawData(owner,&state->model,0,bundle,0x1E8,
        nullptr,pose,bundle->getRawFileAtOffset(0x5CC),nullptr)) {
        delete state;return nullptr;
    }
    stepAnimation(&state->model);
    return state;
}
bool draw_map_destination_models(MapDestinationModels* state,sSaturnPtr program,
    sSaturnPtr entries,s32 count,s32 selected,s_workAreaCopy* owner)
{
    if(!state || program.isNull() || entries.isNull() || count<0 || count>50 ||
       !owner || !owner->m0_fileBundle)return false;
    // Draw06056288: retail rendering excludes the keyboard-only preview.
    if(state->flags&0x40){state->flags&=~0x40u;refresh_destination_flags(*state);}
    auto* bundle=owner->m0_fileBundle;
    if(state->flags&1)LCSItemBox_DrawType0Sub0(bundle,0x1D4,0x5B0);
    if(state->flags&2)LCSItemBox_DrawType0Sub0(bundle,0x1D0,0x5AC);
    if(state->flags&4)LCSItemBox_DrawType0Sub0(bundle,0x1D8,0x5B4);
    if(state->flags&8) {
        pushCurrentMatrix();
        const auto position=readSaturnVec3(program+0x4C94);
        translateCurrentMatrix(position);
        LCSItemBox_DrawType0Sub0(bundle,0x1DC,0x5B8);
        popMatrix();
    }
    if(!(state->flags&0x10))return true;
    // WORLD06055F90: exactly one quad animation tick per destination pass.
    sGunShotTask_UpdateSub4(&state->quad);
    for(s32 i=0;i<count;++i) {
        const auto entry=entries+i*0x24;
        const s16 gate=readSaturnS16(entry);
        if(gate>=0 && !mainGameState.getBit(gate))continue;
        pushCurrentMatrix();
        const auto position=readSaturnVec3(entry+8);
        switch(readSaturnS8(entry+5)) {
        case 0:case 1:
            // Only unlocked type0/1 records advance; selected rises to12,
            // every other record decays to0. Draw uses model+14, not time.
            state->progress[i]=i==selected?std::min(state->progress[i]+1,12):
                std::max(state->progress[i]-1,0);
            translateCurrentMatrix(position);
            state->model.m14=s16(state->progress[i]);
            state->model.m18_drawFunction(&state->model);
            break;
        case 2:
            translateCurrentMatrix(position);
            addObjectToDrawList(bundle->get3DModel(0x3CC));
            break;
        case 3: {
            const auto offset=readSaturnVec3(program+0x4CAC);
            sVec3_FP spritePosition;
            for(int axis=0;axis<3;++axis)spritePosition[axis]=s32(
                u32(position[axis].asS32())+u32(offset[axis].asS32()));
            lagiDrawPoolParticle(&state->quad,&spritePosition,fixedPoint(0x20000),fixedPoint(-0x3000));
            translateCurrentMatrix(position);
            translateCurrentMatrix(offset);
            LCSItemBox_DrawType0Sub0(bundle,0x1E4,0x5C0);
            break;
        }
        default:break; // native switch still balances push/pop for other types
        }
        popMatrix();
    }
    return true;
}
void delete_map_destination_models(MapDestinationModels* state){delete state;}
}
