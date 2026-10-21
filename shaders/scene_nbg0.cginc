// Native NBG0 tile16/4bpp decode. Runs at the Saturn raster, never per
// high-resolution display pixel. Coordinates and line tables are Azel-owned.
float nbgBit(float v,float bit) { return fmod(floor(v/bit),2.0f); }
float nbgWrap(float v,float size) { return v-floor(v/size)*size; }
float nbgFixed(uniform sampler2D raw,float address) {
    address=nbgWrap(address,524288.0f);
    // A native line longword fits exactly in one packed RGBA texel. Avoid
    // repeated byte-lane selection and dependent reads for both halfwords.
    if(fmod(address,4.0f)<0.5f) {
        float4 bytes=rawTexel(raw,address*0.25f);
        float hi=floor(bytes.x*255.0f+0.5f)*256.0f+floor(bytes.y*255.0f+0.5f);
        if(hi>=32768.0f)hi-=65536.0f;
        float lo=floor(bytes.z*255.0f+0.5f)*256.0f+floor(bytes.w*255.0f+0.5f);
        return hi+lo/65536.0f;
    }
    float hi=readBE16(raw,address);
    if(hi>=32768.0f)hi-=65536.0f;
    return hi+readBE16(raw,address+2.0f)/65536.0f;
}
bool nbgWindow(uniform sampler2D raw,float2 pixel,float4 rect,float lineAddress,bool line) {
    if(line) {
        float address=nbgWrap(lineAddress+pixel.y*4.0f,524288.0f);
        rect.x=floor(readBE16(raw,address)/2.0f);
        rect.z=floor(readBE16(raw,address+2.0f)/2.0f);
    }
    return pixel.x>=rect.x && pixel.x<=rect.z && pixel.y>=rect.y && pixel.y<=rect.w;
}
float4 sampleSceneNbg0(uniform sampler2D raw,float2 uv,uniform float4 state[8]) {
    float2 pixel=min(floor(uv*float2(352.0f,224.0f)),float2(351.0f,223.0f));
    float ctl=state[6].x;
    bool en0=nbgBit(ctl,2.0f)>0.5f,en1=nbgBit(ctl,8.0f)>0.5f;
    bool a=nbgWindow(raw,pixel,state[4],state[6].y,nbgBit(state[6].w,1.0f)>0.5f);
    bool b=nbgWindow(raw,pixel,state[5],state[6].z,nbgBit(state[6].w,2.0f)>0.5f);
    a=(nbgBit(ctl,1.0f)>0.5f)?a:!a;
    b=(nbgBit(ctl,4.0f)>0.5f)?b:!b;
    bool visible= en0 && en1 ? (nbgBit(ctl,128.0f)>0.5f ? a && b : a || b)
        : (en0 ? a : (en1 ? b : true));
    if(!visible)return float4(0,0,0,0);
    float3 line=float3(0.0f,pixel.y*state[2].w,state[2].z);
    float lctl=state[3].y;
    bool lx=nbgBit(lctl,2.0f)>0.5f,ly=nbgBit(lctl,4.0f)>0.5f,lz=nbgBit(lctl,8.0f)>0.5f;
    float stride=4.0f*((lx?1.0f:0.0f)+(ly?1.0f:0.0f)+(lz?1.0f:0.0f));
    float address=state[3].x+floor(pixel.y/state[3].z)*stride;
    if(lx){line.x=nbgFixed(raw,address);address+=4.0f;}
    if(ly){line.y=nbgFixed(raw,address)+nbgWrap(pixel.y,state[3].z);address+=4.0f;}
    if(lz)line.z=nbgFixed(raw,address);
    float2 pos=floor(state[2].xy+float2(pixel.x*line.z+line.x,line.y));
    float2 planeSize=state[7].xy;
    pos=float2(nbgWrap(pos.x,planeSize.x*2.0f),nbgWrap(pos.y,planeSize.y*2.0f));
    float plane=floor(pos.x/planeSize.x)+2.0f*floor(pos.y/planeSize.y);
    float base=plane<0.5f?state[1].x:plane<1.5f?state[1].y:plane<2.5f?state[1].z:state[1].w;
    pos=float2(nbgWrap(pos.x,planeSize.x),nbgWrap(pos.y,planeSize.y));
    float page=floor(pos.y/512.0f)*(planeSize.x/512.0f)+floor(pos.x/512.0f);
    float2 tile=floor(float2(nbgWrap(pos.x,512.0f),nbgWrap(pos.y,512.0f))/16.0f);
    float name=readBE16(raw,nbgWrap(base+page*2048.0f+(tile.y*32.0f+tile.x)*2.0f,524288.0f));
    float2 within=float2(nbgWrap(pos.x,16.0f),nbgWrap(pos.y,16.0f));
    if(nbgBit(name,1024.0f)>0.5f)within.x=15.0f-within.x;
    if(nbgBit(name,2048.0f)>0.5f)within.y=15.0f-within.y;
    float scn=state[0].x;
    float character=fmod(name,1024.0f)*4.0f+fmod(scn,4.0f)+floor(fmod(scn,32.0f)/4.0f)*4096.0f;
    float cell=floor(within.y/8.0f)*2.0f+floor(within.x/8.0f);
    float byteAddress=character*32.0f+cell*32.0f+fmod(within.y,8.0f)*4.0f+floor(fmod(within.x,8.0f)/2.0f);
    float packedByte=readByte(raw,nbgWrap(byteAddress,524288.0f));
    float dotIndex=fmod(within.x,2.0f)>0.5f?fmod(packedByte,16.0f):floor(packedByte/16.0f);
    if(dotIndex<0.5f && state[0].z<0.5f)return float4(0,0,0,0);
    float palette=state[0].y*256.0f+floor(name/4096.0f)*16.0f+floor(scn/32.0f)*256.0f+dotIndex;
    return rgb555(readBE16(raw,524288.0f+fmod(palette,2048.0f)*2.0f));
}
