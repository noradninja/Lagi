// Native BTL_A5_3 06056202 / 06056530. The model drawer produces view-space
// hotpoints; the existing flattening service preserves their bone/point order.
void urchinUpdateSub3(s_3dModel*, std::vector<sVec3_FP>&);

static void lagiDrawLathumPart(s_fileBundle* bundle, const sVec3_FP& viewPosition,
                              const sVec3_FP& rotation, u16 mesh) {
    sVec3_FP worldPosition;
    transformAndAddVec(viewPosition,worldPosition,cameraProperties2.m28[0]);
    pushCurrentMatrix();
    translateCurrentMatrix(&worldPosition);
    rotateCurrentMatrixShiftedY(0x08000000);
    pushCurrentMatrix();
    rotateCurrentMatrixZYX(&rotation);
    addObjectToDrawList(bundle->get3DModel(mesh));
    popMatrix();
    popMatrix();
}

static void lagiDrawLathumRig(s_workAreaCopy* owner, LagiLathumRig& rig) {
    // Native phase 0 always draws the emergence model, even if the normal
    // body's hide byte is set. Other phases share the explicit hide guard.
    if (rig.modelPhase!=0 && rig.hideBody) return;
    auto& model=rig.modelPhase==0?rig.emergenceModel:rig.models[0];
    pushCurrentMatrix();
    translateCurrentMatrix(&rig.position);
    rotateCurrentMatrixShiftedY(0x08000000);
    model.m18_drawFunction(&model);
    popMatrix();
    if (rig.modelPhase==0) return;

    urchinUpdateSub3(&model,rig.corePositions);
    for (unsigned group=0;group<rig.groups.size();++group) {
        const auto state=rig.groups[group].state;
        const auto anchor=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3ED4+group));
        const auto bone=readSaturnU8(g_BTL_A5_3->getSaturnPtr(0x060A3FFE + group));
        const auto mesh=readSaturnU16(g_BTL_A5_3->getSaturnPtr(0x060A3FC8+group*6+state*2));
        lagiDrawLathumPart(owner->m0_fileBundle,rig.corePositions.at(anchor),
                          model.m2C_poseData.at(bone).mC_rotation,mesh);
    }
    // Native tail: two attachments share pose bone 3, but use distinct core
    // hotpoint anchors. These are meshes, not additional model hierarchies.
    const auto& rotation=model.m2C_poseData.at(3).mC_rotation;
    lagiDrawLathumPart(owner->m0_fileBundle,rig.corePositions.at(1),rotation,0x94);
    lagiDrawLathumPart(owner->m0_fileBundle,rig.corePositions.at(2),rotation,0xBC);
}
