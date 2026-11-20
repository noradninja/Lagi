// CHSZ=1, CHCN=0/1, PNB=1, CNSM=0 OVPNR character addressing.
// Scalar-only so the production address calculation also runs in host tests.
float rbgOverPatternCharacterBase(float name,float scn)
{
    return (fmod(name,1024.0f)*4.0f+fmod(scn,4.0f)+
        fmod(floor(scn/4.0f),8.0f)*4096.0f)*32.0f;
}
float rbgOverPatternPixelOffset(float name,float x,float y)
{
    const float flip=fmod(floor(name/1024.0f),4.0f);
    float px=frac(floor(x)/16.0f)*16.0f;
    float py=frac(floor(y)/16.0f)*16.0f;
    if(flip>=2.0f)py=py<8.0f?23.0f-py:15.0f-py;
    else if(py>=8.0f)py+=8.0f;
    if(flip==1.0f || flip==3.0f) {
        if(px<8.0f)py+=8.0f;
        px=7.0f-fmod(px,8.0f);
    } else {
        if(px>=8.0f)py+=8.0f;
        px=fmod(px,8.0f);
    }
    return py*8.0f+px;
}
float rbgOverPatternDotAddress(float name,float scn,float x,float y)
{
    return rbgOverPatternCharacterBase(name,scn)+rbgOverPatternPixelOffset(name,x,y);
}
// Four possible pages per plane, eight bounded atlas slots. Integer packed
// values are at most4095, exactly representable in the float uniform.
float rbgDecodedPageSlot(float encoded,float page)
{
    if(page<0.5f)return fmod(encoded,8.0f);
    const float divisor=page<1.5f?8.0f:page<2.5f?64.0f:512.0f;
    return fmod(floor(encoded/divisor),8.0f);
}
