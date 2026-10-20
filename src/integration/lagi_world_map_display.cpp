#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_display.h"
#include "lagi/world_map_motion.h"
#include "3dEngine.h"
#include "kernel/fileBundle.h"

namespace lagi::worldmap {
namespace {
sVec3_FP program_vector(const MapDisplayInput& in,u32 address)
{return readSaturnVec3(in.program+(address-0x06054000));}
void approach_vector(sVec3_FP& value,const sVec3_FP& goal,unsigned profile)
{
    for(int i=0;i<3;++i)value[i]=map_approach_profile(value[i].asS32(),goal[i].asS32(),profile);
}
void spring_vector(sVec3_FP& value,const sVec3_FP& goal,sVec3_FP& velocity)
{
    for(int i=0;i<3;++i){s32 p=value[i].asS32(),v=velocity[i].asS32();
        map_spring_axis(p,goal[i].asS32(),v);value[i]=p;velocity[i]=v;}
}
void record_trail(MapDisplay& s)
{
    // 06054D82: the retail delay-slot TST of literal 1 never skips a record.
    s.trailTick=static_cast<s32>(static_cast<u32>(s.trailTick)+1u);
    s.trailHead=(s.trailHead+1)&15;
    s.trail[s.trailHead]={s.marker.position,s.marker.yaw};
}
void move_marker(MapDisplay& s,sSaturnPtr entry,bool arrivalBusy)
{
    record_trail(s);
    const auto goal=readSaturnVec3(entry+8);
    if(s.marker.flags&2){
        s.marker.position=goal;s.poseCounter=12;s.marker.flags|=0x10;
    }else if(s.marker.flags&1){
        spring_vector(s.marker.position,goal,s.markerVelocity);
        const bool close=vecDistance(s.marker.position,goal)<s.marker.transitionRate;
        if(close&&(readSaturnS8(entry+5)==0||readSaturnS8(entry+5)==1))
            s.poseCounter=std::min(s.poseCounter+1,12);
        else s.poseCounter=std::max(s.poseCounter-1,0);
        bool arrived=true;
        for(int i=0;i<3;++i)arrived&=s.marker.position[i]==goal[i];
        if(arrived){s.poseCounter=12;s.marker.flags|=0x10;}
    }
    s.yawVelocity=(s.marker.flags&1)?0xB60B60:
        map_approach(s.yawVelocity,0x2D82D8,0x1999,0xB60B6,0x12345);
    s.marker.yaw+=s.yawVelocity;
    s.marker.scale=arrivalBusy?MTH_Mul(0xF333,s.marker.scale):
        fixedPoint(map_approach(s.marker.scale.asS32(),0x10000,0xCCC,0x2D82D8,0x41));
    s.marker.poseControl=static_cast<s16>(s.poseCounter);
}
void initialize_position(MapDisplay& s,sSaturnPtr entry)
{
    s.marker.position=readSaturnVec3(entry+8);
    s.desiredTarget=s.target=s.marker.position;
    s.desiredCamera=s.marker.position;
    s.desiredCamera[1]+=0xF000;s.desiredCamera[2]+=0x5000;
    s.camera=s.desiredCamera;
    s.markerVelocity.zeroize();s.cameraVelocity.zeroize();s.yawVelocity=0;
}
void orbit_camera(MapDisplay& s,const MapDisplayInput& in,sSaturnPtr entry)
{
    // 060547C8: signed -3..3 orbit control and bounded velocity acceleration.
    s.orbitControl=std::clamp<int>(s.orbitControl,-3,3);
    const s32 goal=s.orbitControl*0x369D0;
    if(s.orbitVelocity<goal)s.orbitVelocity=std::min(s.orbitVelocity+0x3A41,goal);
    else s.orbitVelocity=std::max(s.orbitVelocity-0x3A41,goal);
    s.orbitAngle=static_cast<s32>(static_cast<u32>(s.orbitAngle)+static_cast<u32>(s.orbitVelocity));
    sVec3_FP offset;
    sMatrix4x3 matrix;initMatrixToIdentity(&matrix);
    if(s.state>=5)offset=program_vector(in,s.orbitControl?0x06058768:0x06058750);
    else {
        offset=readSaturnVec3(entry+0x14)-readSaturnVec3(entry+8);
        translateMatrix(readSaturnVec3(entry+8),&matrix);
    }
    rotateMatrixY(s.orbitAngle,&matrix);
    sVec3_FP cameraGoal;transformAndAddVec(offset,cameraGoal,matrix);
    spring_vector(s.desiredCamera,cameraGoal,s.cameraVelocity);
}
void update_camera_rotation(MapDisplay& s)
{
    // 06054AAA: preserve native fixed-point horizontal length and angle order.
    const auto d=s.target-s.camera;
    const fixedPoint length=MTH_Mul(d[0],d[0])+MTH_Mul(d[2],d[2]);
    const s32 horizontal=sqrt_F(static_cast<s32>(static_cast<u32>(length.asS32())<<4))>>4;
    s.rotation[0]=0x4000000-atan2_FP(horizontal,d[1]);
    s.rotation[1]=static_cast<s32>(0xFC000000u)-atan2_FP(d[2],d[0]);
    s.rotation[2]=0;
}
void update_colors(MapDisplay& s,const MapDisplayInput& in)
{
    s.colorBlend=std::clamp(s.colorBlend+((s.marker.flags&0x40)?1:-1),0,31);
    for(int color=0;color<4;++color){
        u32 packed=0;
        for(int c=0;c<3;++c){
            const int i=color*3+c;
            const s32 a=readSaturnS8(in.program+(0x06058774-0x06054000)+i);
            const s32 b=readSaturnS8(in.program+(0x06058780-0x06054000)+i);
            packed|=u32(u8(a+((b-a)*s.colorBlend)/31))<<(c*8);
        }
        s.marker.lightColors[color]=packed;
    }
}
}

bool update_map_display(MapDisplay& s,const MapDisplayInput& in)
{
    if(in.program.isNull()||in.targetTable.isNull()||in.targetCount<=0)return false;
    const s32 selected=s.state==0?in.cameraSelection:s.selectedTarget;
    if(selected<0||selected>=in.targetCount)return false;
    if(s.state==3&&!in.arrivalReady)return false;
    s.selectedTarget=selected;
    const auto entry=in.targetTable+selected*0x24;
    // Native state 0..6 controller at 06054F9E. Debug keyboard edits from
    // 06054308 are intentionally not part of this retail service.
    switch(s.state){
    case 0:
        initialize_position(s,entry);s.marker.flags|=2;s.marker.scale=0;
        move_marker(s,entry,in.arrivalBusy);s.desiredTarget=s.marker.position;
        s.orbitControl=0;s.orbitVelocity=0;s.orbitAngle=0;s.state=1;break;
    case 1:
        move_marker(s,entry,in.arrivalBusy);s.desiredTarget=s.marker.position;
        orbit_camera(s,in,entry);
        approach_vector(s.camera,s.desiredCamera,0);approach_vector(s.target,s.desiredTarget,1);break;
    case 2:
        move_marker(s,entry,in.arrivalBusy);s.desiredTarget=s.marker.position;
        orbit_camera(s,in,entry);
        s.camera[0]=map_approach_profile(s.camera[0].asS32(),s.target[0].asS32(),1);
        s.camera[1]=map_approach_profile(s.camera[1].asS32(),s.target[1].asS32(),0);
        s.camera[2]=map_approach_profile(s.camera[2].asS32(),s.target[2].asS32(),1);
        approach_vector(s.target,s.desiredTarget,1);break;
    case 3:{
        move_marker(s,entry,in.arrivalBusy);s.desiredTarget=in.arrivalPosition;s.desiredTarget[1]-=0x5000;
        sVec3_FP goal=readSaturnVec3(entry+8);
        goal[0]+=(goal[0]>s.desiredTarget[0])?0x14000:-0x14000;
        goal[1]+=0x14000;goal[2]=s.desiredTarget[2];
        spring_vector(s.desiredCamera,goal,s.cameraVelocity);
        for(int i=0;i<3;++i)s.camera[i]=map_approach_profile(s.camera[i].asS32(),s.desiredCamera[i].asS32(),i==1?5:0);
        approach_vector(s.target,s.desiredTarget,1);break;}
    case 4:
        s.state=1;s.orbitControl=0;s.orbitVelocity=0;s.orbitAngle=0;
        move_marker(s,entry,in.arrivalBusy);
        if(s.previousState==1)s.marker.flags|=5;
        else {s.marker.flags&=~4;s.marker.flags|=2;}
        if(s.marker.flags&8)s.marker.flags&=~8;
        else {approach_vector(s.camera,s.desiredCamera,0);approach_vector(s.target,s.desiredTarget,1);}
        break;
    case 5:
        s.orbitControl=0;s.orbitVelocity=0;s.orbitAngle=0;
        s.desiredCamera=program_vector(in,0x06058750);s.desiredTarget=program_vector(in,0x0605875C);
        s.state=6;break;
    case 6:
        orbit_camera(s,in,entry);approach_vector(s.camera,s.desiredCamera,2);
        approach_vector(s.target,s.desiredTarget,2);break;
    default:break;
    }
    if(s.marker.flags&0x10){s.markerVelocity.zeroize();s.marker.flags&=~0x17;}
    s.previousState=s.state;
    update_camera_rotation(s);update_colors(s,in);
    s.trailCount=std::clamp(s.trailCount+((s.marker.flags&4)?1:-1),0,15);
    return true;
}

void apply_map_camera_and_light(const MapDisplay& s)
{
    sVec3_S16 rotation;
    for(int i=0;i<3;++i)rotation[i]=s.rotation[i].asS32()>>16;
    updateEngineCamera(&cameraProperties2,s.camera,rotation);
    const int pitch=s.marker.lightAngle>>16,yaw=s.lightYaw>>16;
    sVec3_FP direction;
    direction[0]=-MTH_Mul_5_6(0x10000,getCos(pitch),getSin(yaw));
    direction[1]=MTH_Mul(0x10000,getSin(pitch));
    direction[2]=-MTH_Mul_5_6(0x10000,getCos(pitch),getCos(yaw));
    sVec3_FP viewDirection;transformVecByCurrentMatrix(direction,viewDirection);
    setupLight(viewDirection[0],viewDirection[1],viewDirection[2],s.marker.lightColors[0]);
    generateLightFalloffMap(s.marker.lightColors[1],s.marker.lightColors[2],s.marker.lightColors[3]);
}

bool draw_map_trail(const MapDisplay& s,s_workAreaCopy* owner)
{
    // WORLD 06055128: newest ring element first, walking backward with wrap.
    // The authored trail is MCB slot 03DC, not a copied main marker model.
    if(s.trailCount<0||s.trailCount>15||s.trailHead<0||s.trailHead>=16)return false;
    if(!s.trailCount)return true;
    if(!owner||!owner->m0_fileBundle)return false;
    auto* model=owner->m0_fileBundle->get3DModel(0x3DC);
    if(!model)return false;
    int index=s.trailHead;
    for(int count=0;count<s.trailCount;++count){
        pushCurrentMatrix();
        translateCurrentMatrix(s.trail[index].position);
        rotateCurrentMatrixY(s.trail[index].yaw);
        addObjectToDrawList(model);
        popMatrix();
        index=(index+15)&15;
    }
    return true;
}
}
