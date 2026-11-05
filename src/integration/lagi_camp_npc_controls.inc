// Native CAMP0606FF30 control map and06070ABC patrol. Included before the
// verified shared NPC task bodies. Host packing never stands in for Saturn bytes.
#include "lagi/camp_npc_patrol.h"
#include "town/townEdge.h"
#include "audio/systemSounds.h"

void scheduleNPCAnimationFromTable(sNPC*,s8,u8);
static sAnimationData* lagiCampNpcAnimation(sNPC* npc,s32 index)
{
    const auto entry=npc->m30_animationTable+index*4;
    auto* bundle=readSaturnU16(entry)?dramAllocatorEnd[0].mC_fileBundle->m0_fileBundle:npc->m0_fileBundle;
    return bundle->getAnimation(readSaturnU16(entry+2));
}
static void lagiCampNpcDecay(sNPC* npc)
{
    npc->m20_lookAtAngle[1]=MTH_Mul(npc->m20_lookAtAngle[1],0xB333);
}
static void lagiCampNpcPosePlayback(sNPC* npc)
{
    lagiCampNpcDecay(npc);
    const auto table=npc->m18;
    if(npc->m150_inputX>=readSaturnS32(table))npc->m150_inputX=0;
    const auto entry=readSaturnEA(table+4)+npc->m150_inputX*12;
    for(unsigned i=0;i<3;++i) {
        npc->mE8.m0_position[i]=s32(readSaturnS16(entry+i*2))*16;
        npc->mE8.mC_rotation[i]=s32(readSaturnS16(entry+6+i*2))*65536;
    }
    ++npc->m150_inputX;
}
static void lagiCampNpcWeightedAnimation(sNPC* npc)
{
    lagiCampNpcDecay(npc);
    if(npc->mE_controlState!=0)return;
    const s8 index=static_cast<s8>(u16(npc->m14C_inputFlags)&255u); // Native+14D
    const auto entry=npc->m18+index*16;
    const u32 roll=performModulo2(readSaturnU16(entry+12),randomNumber());
    const s8 animation=roll<u32(readSaturnS16(entry+10))?
        readSaturnS8(readSaturnEA(entry)+roll):readSaturnS8(entry+8);
    scheduleNPCAnimationFromTable(npc,animation,2);
}
static void lagiCampNpcPatrol(sNPC* npc)
{
    namespace p=lagi::camp::patrol;
    if(npc->m84.m2C_collisionSetupIndex==1)
        npc->mE8.m0_position+=npc->m84.m58_collisionSolveTranslation;
    p::State state{static_cast<s8>(u16(npc->m14C_inputFlags)>>8),npc->m150_inputX};
    p::Frame frame{npc->mE8.m0_position[0].asS32(),npc->mE8.m0_position[2].asS32(),
        npc->mE8.mC_rotation[1].asS32(),npc->mE8.m24_stepRotation[1].asS32(),
        npc->mE8.m30_stepTranslation[2].asS32(),npc->m20_lookAtAngle[1].asS32(),
        npc->m1C,npc->m2C_currentAnimation.asS32(),npc->mE_controlState};
    const auto table=npc->m18;
    const p::Bounds bounds{readSaturnS32(table),readSaturnS32(table+4),readSaturnS32(table+8),readSaturnS32(table+12)};
    p::advance(state,frame,bounds,[](){return randomNumber();},
        [&](s32 animation){playAnimationGeneric(&npc->m34_3dModel,lagiCampNpcAnimation(npc,animation),5);},
        [&](s32 animation){return static_cast<s16>(lagiCampNpcAnimation(npc,animation)->m4_numFrames);},
        [](s32 n,s32 d){return FP_Div(n,d).asS32();},
        [&](const p::Frame& current){
            npc->mE8.mC_rotation[1]=current.yaw;
            npc->mE8.m30_stepTranslation[2]=current.speed;
            stepNPCForward(&npc->mE8);
        });
    npc->m14C_inputFlags=std::bit_cast<s16>(u16((u16(u8(state.action))<<8)|(u16(npc->m14C_inputFlags)&255u)));
    npc->m150_inputX=state.timer;
    npc->mE8.mC_rotation[1]=frame.yaw;npc->mE8.m24_stepRotation[1]=frame.stepYaw;
    npc->mE8.m30_stepTranslation[2]=frame.speed;npc->m20_lookAtAngle[1]=frame.lookYaw;
    npc->m2C_currentAnimation=frame.animation;npc->mE_controlState=frame.control;
}

// CAMP0606F3F0. Native cleared work area adapted explicitly; do not call the
// shared initializer's unimplemented mode2 or its hard-coded mode3 receiver.
static void lagiCampNpcInit(sNPC* npc,sSaturnPtr arg)
{
    const u8 slot=readSaturnU8(arg+0x20);
    npcData0.m70_npcPointerArray[slot].workArea=npc;
    npcData0.m70_npcPointerArray[slot].pNPC=npc;
    npc->mC=0;npc->mF=readSaturnU8(arg+0x21)&0x80;
    npc->mD=readSaturnU8(arg+0x21)&0x3F;npc->mE_controlState=0;
    npc->m10_InitPtr=arg;npc->m18=readSaturnEA(arg+0x30);
    npc->m1C=readSaturnS32(arg+0x28);npc->m30_animationTable=readSaturnEA(arg+0x2C);
    npc->m20_lookAtAngle={};npc->m28_animationLeftOver=0;npc->m2C_currentAnimation=0;
    npc->mE8={};npc->mE8.m0_position=readSaturnVec3(arg+8);npc->mE8.mC_rotation=readSaturnVec3(arg+0x14);
    npc->m148_savedAngle=0;npc->m14C_inputFlags=0;
    npc->m14E=npc->mD==4?0x100:0; // Native MOV.B1 at+14E in a cleared big-endian word.
    npc->m150_inputX=npc->m154_inputY=0;
    for(auto& value:npc->m158_animQueue)value=0;
    npc->m178=npc->m179=npc->m17A=npc->m17B=0;
    switch(npc->mD) {
    case 0:npc->m14_updateFunction=&lagiCampNpcDecay;break;
    case 1:npc->m14_updateFunction=&lagiCampNpcPosePlayback;npc->mE_controlState=1;break;
    case 2:npc->m14_updateFunction=&lagiCampNpcPatrol;break;
    case 3:
        // The two authored Camp mode3 entities carry this original function.
        assert(npc->m18.m_offset==0x06070EB8);
        npc->m14_updateFunction=&updateEdgePosition;break;
    case 4:npc->m14_updateFunction=&lagiCampNpcWeightedAnimation;break;
    default:assert(0);npc->m14_updateFunction=nullptr;break;
    }
    npc->m84={};npc->m84.m30_pPosition=&npc->mE8.m0_position;
    npc->m84.m34_pRotation=&npc->mE8.mC_rotation;npc->m84.m38_pOwner=npc;
    npc->m84.m3C_scriptEA=readSaturnEA(arg+0x38);
    const auto collision=readSaturnU16(arg+0x36);
    npc->m84.m40=collision?npc->m0_fileBundle->getCollisionModel(collision):nullptr;
    setCollisionSetup(&npc->m84,readSaturnU8(arg+0x34));
    setCollisionBounds(&npc->m84,readSaturnVec3(arg+0x3C),readSaturnVec3(arg+0x48));
}
