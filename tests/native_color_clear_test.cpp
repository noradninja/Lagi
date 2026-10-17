#include <cstdint>
#include <cstring>
#include <cassert>
#include <vector>
#include <cstdio>
constexpr int SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR=6;
int accepted=0,finished=0,submits=0,waits=0;
void* pending=nullptr;unsigned pendingWidth,pendingHeight;int pendingStride;
int sceGxmTransferFill(unsigned color,int format,void* pixels,unsigned x,unsigned y,unsigned width,unsigned height,int stride,void* sync,unsigned flags,void* notify){
    assert(!color && format==6 && !x && !y && !sync && !flags && !notify && stride==int(width*4));
    ++submits;pending=pixels;pendingWidth=width;pendingHeight=height;pendingStride=stride;return accepted;
}
int sceGxmTransferFinish(){++waits;if(finished<0)return finished;std::memset(pending,0,pendingHeight*pendingStride);return 0;}
#include "clear.inc"
int main(){
    for(auto dimensions:{std::vector<int>{960,544,960},std::vector<int>{480,272,512}}){
        const auto width=dimensions[0],height=dimensions[1],pitch=dimensions[2];
        std::vector<std::uint32_t> buffer(pitch*height+8,0xDEADBEEF);
        accepted=finished=0;submits=waits=0;
        assert(clearNativeSceneColorBuffer(buffer.data(),width,height,pitch) && submits==1 && waits==1);
        for(int i=0;i<pitch*height;++i)assert(buffer[i]==0);assert(buffer.back()==0xDEADBEEF);
        std::fill(buffer.begin(),buffer.end(),0xDEADBEEF);accepted=-1;waits=0;
        assert(clearNativeSceneColorBuffer(buffer.data(),width,height,pitch) && !waits);
        for(int i=0;i<pitch*height;++i)assert(buffer[i]==0);assert(buffer.back()==0xDEADBEEF);
        accepted=0;finished=-1;assert(!clearNativeSceneColorBuffer(buffer.data(),width,height,pitch));
    }
    assert(!clearNativeSceneColorBuffer(nullptr,1,1,1));
    puts("PASS: production color-clear submits exact pitch/height and waits before consumption; rejected transfer uses bounded CPU fallback; failed completion rejects frame. Mocked GPU, no performance claim.");
}
