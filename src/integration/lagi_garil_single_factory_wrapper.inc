// Existing single formation supplies the same live state to the shared factory.
static p_workArea BTL_A5_createEnemySingle(sLagiGarilSingleFormation* parent,s32 index,s32 isLead) {
    const LagiGarilSingleGroupBinding binding{&parent->mE0_entityGroup,
        &parent->mF0_flag,&parent->mF1_pad5[1],parent->mF4_entityPositions,parent,
        [](void* context,int entity)->sVec3_FP {
            auto* formation=static_cast<sLagiGarilSingleFormation*>(context);
            return formation->m_lagiLayouts[formation->m_lagiLayout][entity];
        }};
    return lagiCreateGarilSingleActor(parent,binding,index,isLead);
}
