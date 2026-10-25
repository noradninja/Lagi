// BTL_T0 0609F5B4..0609F644. Use native matrix/model entry points; no
// replacement geometry, per-frame allocation or additional GXM pass.
void lagiDrawBattleEnemyModels(std::uint32_t& flags, std::int16_t lightColor,
    const sVec3_FP& position, const sVec3_FP& rotation,
    std::vector<s_3dModel>& models)
{
    if (flags&0x800000u) setupConditionalLightColor(lightColor);
    for (auto& model:models) {
        pushCurrentMatrix();
        translateCurrentMatrix(&position);
        rotateCurrentMatrixYXZ(&rotation);
        model.m18_drawFunction(&model);
        popMatrix();
    }
    // Native reads the LIVE flag after all model callbacks, not a saved bool.
    if (flags&0x800000u) {
        flags&=~0x800000u;
        clearLightColor();
    }
}
