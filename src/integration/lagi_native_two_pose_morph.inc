// MAIN 0601507A / 06014FF4 / 060135C8 / 06013676.
// Two authored shape sources, independent of animation rotation/scale.
#include "processModel.h"
#include "lagi/lagi_render_bridge.h"
static s32 lagiMorphPair32(s32 a, s32 b, s32 weight)
{
    return static_cast<s32>((static_cast<int64_t>(a) * weight +
        static_cast<int64_t>(b) * (0x10000 - weight)) >> 16);
}
static s16 lagiMorphPair16(s16 a, s16 b, s32 weight)
{
    // Native MAC.W uses the weights truncated to signed 2.14 first.
    return static_cast<s16>((static_cast<s32>(a) * (weight >> 2) +
        static_cast<s32>(b) * ((0x10000 - weight) >> 2)) >> 14);
}
static void lagiMorphPairHierarchy(sModelHierarchy* dst,
    const sModelHierarchy* a, const sModelHierarchy* b, s32 weight)
{
    for (; dst && a && b; dst=dst->m8_nextNode, a=a->m8_nextNode, b=b->m8_nextNode) {
        if (dst->m0_3dModel && a->m0_3dModel && b->m0_3dModel) {
            auto& d=*dst->m0_3dModel;
            const auto& x=*a->m0_3dModel;
            const auto& y=*b->m0_3dModel;
            if (d.m4_numVertices && d.m8_vertices.size()==x.m8_vertices.size() &&
                d.m8_vertices.size()==y.m8_vertices.size() &&
                d.mC_Quads.size()==x.mC_Quads.size() && d.mC_Quads.size()==y.mC_Quads.size()) {
                d.m0_radius=lagiMorphPair32(x.m0_radius, y.m0_radius, weight);
                for (size_t i=0;i<d.m8_vertices.size();++i)
                    for (int axis=0;axis<3;++axis)
                        d.m8_vertices[i][axis]=lagiMorphPair16(x.m8_vertices[i][axis],y.m8_vertices[i][axis],weight);
                for (size_t q=0;q<d.mC_Quads.size();++q) {
                    auto& out=d.mC_Quads[q];
                    const auto& xa=x.mC_Quads[q].m14_extraData;
                    const auto& ya=y.mC_Quads[q].m14_extraData;
                    const unsigned type=(out.m8_lightingControl>>8)&3;
                    if (!type || out.m14_extraData.size()!=xa.size() || xa.size()!=ya.size()) continue;
                    for (size_t i=0;i<out.m14_extraData.size();++i)
                        for (int axis=0;axis<3;++axis) {
                            out.m14_extraData[i].m0_normals[axis]=lagiMorphPair16(xa[i].m0_normals[axis],ya[i].m0_normals[axis],weight);
                            if (type==2)
                                out.m14_extraData[i].m6_colors[axis]=lagiMorphPair16(static_cast<s16>(xa[i].m6_colors[axis]),static_cast<s16>(ya[i].m6_colors[axis]),weight);
                        }
                }
                d.m_vertexBuffersDirty=true;
                // Published frames retain immutable snapshots; retire only
                // this changed cache entry before its next native submission.
                lagi::azel_bridge::retire_native_model(&d);
            }
        }
        if (dst->m4_subNode && a->m4_subNode && b->m4_subNode)
            lagiMorphPairHierarchy(dst->m4_subNode,a->m4_subNode,b->m4_subNode,weight);
    }
}
static void lagiNativeTwoPoseMorph(npcFileDeleter* owner, s_3dModel* model,
    sSaturnPtr a, sSaturnPtr b, s32 weight)
{
    // Native helper deliberately ignores spring overshoot outside [0,1].
    if (weight<0 || weight>0x10000) return;
    auto* bundle=owner->m0_fileBundle;
    lagiMorphPairHierarchy(model->m4_pModelFile->getModelHierarchy(model->mC_modelIndexOffset),
        bundle->getModelHierarchy(readSaturnU16(a)),
        bundle->getModelHierarchy(readSaturnU16(b)),weight);
    const auto* x=bundle->getStaticPose(readSaturnU16(a+2),model->m12_numBones);
    const auto* y=bundle->getStaticPose(readSaturnU16(b+2),model->m12_numBones);
    if (!x || !y) return;
    for (unsigned i=1;i<model->m12_numBones;++i)
        for (int axis=0;axis<3;++axis)
            model->m2C_poseData[i].m0_translation[axis]=lagiMorphPair32(
                x->m0_bones[i].m0_translation[axis],y->m0_bones[i].m0_translation[axis],weight);
}
