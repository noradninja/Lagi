// MAIN06012674 -> 060125E8: mark the current authored map cell.
static void fieldTerrainCallback(s32 subFieldIndex, sVec3_FP* pPos, sVec3_FP*)
{
    auto* field=getFieldTaskPtr();
    if(!pPos || !field || !gCommonFile || !gCommonFile->m_data ||
        gCommonFile->m_base!=0x200000 ||
        gCommonFile->m_dataSize<0xA388+23*4 || subFieldIndex<0) return;
    const int fieldIndex=field->m2C_currentFieldIndex;
    if(fieldIndex<0 || fieldIndex>=23) return;
    const auto range=[](const auto& p,std::size_t bytes) {
        return p.m_offset>=gCommonFile->m_base &&
            std::size_t(p.m_offset-gCommonFile->m_base)<=gCommonFile->m_dataSize &&
            bytes<=gCommonFile->m_dataSize-std::size_t(p.m_offset-gCommonFile->m_base);
    };
    const auto root=gCommonFile->getSaturnPtr(0x20A388)+fieldIndex*4;
    if(!range(root,4)) return;
    const auto table=readSaturnEA(root);
    if(!range(table,4) || std::uint64_t(subFieldIndex)*4+4>gCommonFile->m_dataSize) return;
    const auto slot=table+static_cast<u32>(subFieldIndex)*4u;
    if(!range(slot,4)) return;
    const auto record=readSaturnEA(slot);
    if(!range(record,10)) return;
    const auto rectangle=readSaturnEA(record);
    if(!range(rectangle,4)) return;
    const s32 px=(*pPos)[0].asS32(),pz=(*pPos)[2].asS32();
    // Preserve native truncating division followed by explicit sign adjustment,
    // including exact negative cell boundaries. NEG wraps at INT32_MIN on SH2.
    const s32 negZ=static_cast<s32>(0u-static_cast<u32>(pz));
    const int x=px/0x200000-readSaturnS8(rectangle)-(px<0?1:0);
    const int y=negZ/0x200000-readSaturnS8(rectangle+1)-(pz>0?1:0);
    const int width=readSaturnS8(rectangle+2),height=readSaturnS8(rectangle+3);
    if(x<0 || y<0 || x>=width || y>=height) return;
    const int flag=readSaturnS16(record+8)+y*width+x;
    // Native helper acknowledges an in-bounds cell but only mutates this
    // exploration range. Do not overwrite adjacent story/item/exit flags.
    if(flag>=0x95A && flag<0x10D8)
        mainGameState.setBit566(static_cast<u32>(flag));
}
