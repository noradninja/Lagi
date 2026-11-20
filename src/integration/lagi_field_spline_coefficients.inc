// FLD_A7 06077488..06077636 (shared A3 06078EBC).
static void fieldSpline_precompute(s32 count,s32* arcLen,s32* src,s32* y2)
{
    auto* scratch=static_cast<s32*>(allocateHeap(static_cast<u32>(count)*8u));
    if(!scratch)return;
    auto* d=scratch;auto* h=scratch+count;
    const auto sub=[](s32 a,s32 b){return static_cast<s32>(static_cast<u32>(a)-static_cast<u32>(b));};
    const auto sh=[](s32 a,unsigned bits){return static_cast<s32>(static_cast<u32>(a)<<bits);};
    const auto mul=[](s32 a,s32 b){return MTH_Mul(fixedPoint(a),fixedPoint(b)).asS32();};
    const auto div=[](s32 a,s32 b){return FP_Div(a,fixedPoint(b)).asS32();};
    y2[0]=0;y2[count-1]=0;
    for(s32 i=0;i<count-1;++i) {
        h[i]=sub(arcLen[i+1],arcLen[i]);
        d[i+1]=div(sub(src[i+1],src[i]),sh(h[i],4));
    }
    y2[1]=sub(d[2],d[1]);
    d[1]=sh(sub(arcLen[2],arcLen[0]),1);
    for(s32 i=1;i<count-2;++i) {
        const s32 ratio=div(h[i],d[i]);
        y2[i+1]=sub(sub(d[i+2],d[i+1]),mul(y2[i],ratio));
        d[i+1]=sub(sh(sub(arcLen[i+2],arcLen[i]),1),mul(h[i],ratio));
    }
    y2[count-2]=sub(y2[count-2],mul(h[count-2],sh(y2[count-1],4)));
    for(s32 k=count-2;k>0;--k)
        y2[k]=div(sub(y2[k],mul(h[k],sh(y2[k+1],4))),sh(d[k],4));
    freeHeap(scratch);
}
