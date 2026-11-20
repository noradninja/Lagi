// PMOD mesh and half-transparency are independent, shared by every solid
// native polygon/line. Opaque non-mesh HUD polygons retain their old program.
static SceGxmFragmentProgram* nativeSolidFragmentProgram(unsigned pmod)
{
    const bool half=lagi::vdp1_half_transparent(pmod);
    if(pmod&0x100u)return half?g_nativeBlendPrograms[BlendSolidMesh]:g_solidMeshFragmentProgram;
    if(half)return LAGI_VDP2_SPRITE_COMPOSITE?g_nativeBlendPrograms[BlendSolid]:g_fadeFragmentProgram;
    return g_probeFragmentProgram;
}
