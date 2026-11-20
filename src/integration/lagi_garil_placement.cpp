#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/garil_placement.h"
#include "field/field_a5/o_fld_a5.h"
#include "field/fieldCamera.h"
#include "field/fieldDragon.h"
#include "trigo.h"

void buildDragonRotationMatrix(s_dragonTaskWorkArea_48*,sVec3_FP*);

bool lagiAzelTryGarilPlacement(sSaturnPtr target)
{
    if (!gFLD_A5 || target.m_file!=gFLD_A5 ||
        (target.m_offset!=0x0605C020 && target.m_offset!=0x0605C054)) return false;
    auto* field=getFieldTaskPtr()->m8_pSubFieldData;
    auto* dragon=field->m338_pDragonTask;
    // MAIN 0600AE68 invalidates a live light-wing task. Its factory is still
    // unported: decline that configuration before changing any native state.
    if (dragon->mB8_lightWingEffect) return false;
    const auto word=[](u32 address){return readSaturnS32(gFLD_A5->getSaturnPtr(address));};
    const bool first=target.m_offset==0x0605C020;
    const sVec3_FP position={word(first?0x0605C088:0x0605C0A0),word(0x0605C08C),
                            word(first?0x0605C090:0x0605C0A4)};
    const sVec3_FP rotation={0,word(0x0605C094),0};
    // A5 0607C584 and 0607FB0C: position/angles, rider rotation at +18,
    // native rotation-matrix producer, then the active camera's yaw.
    dragon->m8_pos=position;
    dragon->m20_angle=rotation;
    if (dragon->mBC) dragon->mBC[2]=rotation;
    buildDragonRotationMatrix(&dragon->m48,&dragon->m20_angle);
    getFieldCameraStatus()->mC_rotation[1]=dragon->m20_angle[1];

    // A5 06069AE8 resets slot zero (not necessarily the active slot).
    // It deliberately does not decay, integrate or clear camera springs.
    auto* camera=&field->m334->m3E4_cameraSlots[0];
    lagiAzelPrepareGarilCamera(camera,dragon);
    camera->m24_distanceToDestination=camera->m40.asS32();
    camera->mC_rotation[0]=camera->mC_rotation[0].normalized();
    camera->m18=camera->m18.normalized();
    fixedPoint pitch=camera->mC_rotation[0]+camera->m18;
    if (pitch < -0x31C71C7) {
        camera->mC_rotation[0]=-camera->m18-0x31C71C7;pitch=-0x31C71C7;
    } else if (pitch > 0x31C71C7) {
        camera->mC_rotation[0]=-camera->m18+0x31C71C7;pitch=0x31C71C7;
    }
    const auto sine=getSin(pitch.getInteger()&0xFFF);
    const auto cosine=getCos(pitch.getInteger()&0xFFF);
    const int yaw=camera->mC_rotation[1].getInteger()&0xFFF;
    const sVec3_FP offset={
        -MTH_Mul_5_6(camera->m24_distanceToDestination,cosine,getSin(yaw)),
         MTH_Mul(camera->m24_distanceToDestination,sine),
        -MTH_Mul_5_6(camera->m24_distanceToDestination,cosine,getCos(yaw))};
    camera->m0_position=dragon->m8_pos-offset;
    return true;
}
