// CHSZ=1, CHCN=1, PNB=1, CNSM=0 OVPNR character addressing.
// Scalar-only so the production address calculation also runs in host tests.
float rbgOverPatternDotAddress(float name,float scn,float x,float y)
{
    const float character=fmod(name,1024.0f)*4.0f+fmod(scn,4.0f)+
        fmod(floor(scn/4.0f),8.0f)*4096.0f;
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
    return character*32.0f+py*8.0f+px;
}
