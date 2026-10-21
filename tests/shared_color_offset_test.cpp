#include "lagi/shared_color_offset.h"
#include <cassert>
int main(){
    for(int v=-256;v<=255;++v){
        assert(lagi::shared_color_offset(127,16,v,v,v,v,v,v));
        assert(!lagi::shared_color_offset(127,16,v,v,v,v+1,v,v));
        assert(!lagi::shared_color_offset(63,16,v,v,v,v,v,v));
    }
    assert(lagi::shared_color_offset(127,0,-256,1,2,5,6,7));
    assert(lagi::shared_color_offset(127,127,-256,1,2,5,6,7));
    assert(!lagi::shared_color_offset(127,16,0,0,0,0,1,0));
    assert(!lagi::shared_color_offset(127,16,0,0,0,0,0,1));
}
