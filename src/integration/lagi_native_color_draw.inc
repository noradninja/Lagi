// Shared native color-animation hierarchy draw variants. Keep pose stepping,
// frame selection and model ownership in Azel; capture only authored colors.
static void lagiDrawColorHierarchy(sModelHierarchy* node,s_3dModel* model,unsigned& bone)
{
    while(node && bone<model->m12_numBones) {
        const unsigned current=bone++;
        pushCurrentMatrix();
        if(model->m8&2) {
            multiplyCurrentMatrix(&model->m3C_boneMatrices[current]);
        } else {
            auto& pose=model->m2C_poseData[current];
            translateCurrentMatrix(&pose.m0_translation);
            rotateCurrentMatrixZYX(&pose.mC_rotation);
            if(model->mA_animationFlags&0x20) {
                scaleCurrentMatrixRow0(pose.m18_scale[0]);
                scaleCurrentMatrixRow1(pose.m18_scale[1]);
                scaleCurrentMatrixRow2(pose.m18_scale[2]);
            }
        }
        if(node->m0_3dModel)
            addObjectToDrawListWithColorAnimation(node->m0_3dModel,
                lagi::nativeColorBone(model->m38_pColorAnim,model->m14,current));
        if(model->m40) {
            auto& definition=(*model->m40)[current];
            auto& output=model->m44_hotpointData[current];
            for(unsigned i=0;i<definition.m4_count;++i) {
                sVec3_FP input=definition.m0[i].m4;
                transformAndAddVecByCurrentMatrix(&input,&output[i]);
            }
        }
        if(node->m4_subNode)lagiDrawColorHierarchy(node->m4_subNode,model,bone);
        popMatrix();node=node->m8_nextNode;
    }
}
static void lagiDrawColorAnimatedModel(s_3dModel* model)
{
    // Original colored variants only submit the object (not billboard) branch.
    if(!(model->m8&1))return;
    unsigned bone=0;
    lagiDrawColorHierarchy(model->m4_pModelFile->getModelHierarchy(
        model->mC_modelIndexOffset),model,bone);
}
