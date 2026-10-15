#include "lagi/azel_map_menu.h"
#include <cassert>
#include <cstdio>
#include <vector>
using s32=std::int32_t;
using u32=std::uint32_t;
struct fixedPoint {
    s32 value=0;
    fixedPoint()=default;
    fixedPoint(s32 v):value(v){}
    s32 asS32()const{return value;}
};
using sVec3_FP=std::array<fixedPoint,3>;
struct sMatrix4x3 {fixedPoint m[3][4];};
struct Dragon {sVec3_FP m8_pos,m20_angle;} dragon;
struct Subfield {Dragon* m338_pDragonTask=&dragon;} subfield;
struct Field {int m2C_currentFieldIndex=1,m2E_currentSubFieldIndex=0;Subfield* m8_pSubFieldData=&subfield;} field;
Field* fieldPtr=&field;
Field* getFieldTaskPtr(){return fieldPtr;}
std::array<unsigned char,0x22000> common{};
struct Common {
    unsigned char* m_data=common.data();
    unsigned m_base=0x200000,m_dataSize=unsigned(common.size());
    unsigned char* getSaturnPtr(unsigned address){assert(address>=m_base && address-m_base<m_dataSize);return m_data+address-m_base;}
} commonObject;
Common* gCommonFile=&commonObject;
struct {unsigned mode=0;unsigned readPackedBits(unsigned a,unsigned b){assert(a==0 && b==2);return mode;}} mainGameState;
unsigned readSaturnU8(unsigned char* p){return *p;}
s32 readSaturnS32(unsigned char* p){return s32((u32(p[0])<<24)|(u32(p[1])<<16)|(u32(p[2])<<8)|p[3]);}
s32 intDivide(s32 denominator,s32 numerator){assert(denominator==0x10000);return numerator/denominator;}
std::vector<int> rotations;
void initMatrixToIdentity(sMatrix4x3* m){*m={};rotations.push_back(0);}
void rotateMatrixShiftedY(fixedPoint angle,sMatrix4x3* m){assert(angle.asS32()==0x1234567);m->m[0][2]=111;rotations.push_back(1);}
void rotateMatrixShiftedX(fixedPoint angle,sMatrix4x3* m){assert(angle.asS32()==0x2345678);m->m[1][2]=222;rotations.push_back(2);}
s32 atan2_FP(s32 y,s32 x){assert(y==111 && x==222);rotations.push_back(3);return 0x3456789;}
#include "map_context_production.inc"
void put(unsigned offset,u32 value){for(int i=3;i>=0;--i){common[offset+i]=value&255;value>>=8;}}
int main()
{
    // Distinct live position and angles expose wrong axis/heading consumers.
    dragon.m8_pos={0x24000,0x34000,0x54000};
    dragon.m20_angle={0x2345678,0x1234567,0x7777777};
    for(unsigned f : {1u,15u,18u})put(0xE848+f*4,0x201000);
    for(unsigned slot=0;slot<16;++slot)put(0x1000+slot*4,0x201100);
    put(0x1100,0x201200);put(0x1104,0x201220);
    std::memcpy(common.data()+0x1200,"MAP_A20.SCB",12);
    std::memcpy(common.data()+0x1220,"MAP_A20.PNB",12);
    common[0x1108]=0xFF;common[0x1109]=0x12;common[0x110A]=4;common[0x110B]=31;
    LagiAzelMapContext out;
    assert(lagiAzelResolveCurrentMapContext(out));
    assert(out.field==1 && out.subfield==0 && out.x==2 && out.y==1029);
    assert(out.heading==0x1234567 && rotations.empty());
    assert(out.descriptor.nativeAddress==0x201100 && out.descriptor.presentation[2]==4);
    dragon.m8_pos[0]=-0x24000;
    assert(lagiAzelResolveCurrentMapContext(out) && out.x==-2); // Truncate, not floor.
    dragon.m8_pos[0]=0x24000;
    unsigned cases=0;
    for(int f : {15,18})for(int s : {0,1,6,9,0x17,0x1A})for(unsigned zone : {2u,15u,16u}) {
        field.m2C_currentFieldIndex=f;field.m2E_currentSubFieldIndex=s;
        common[0x161F4+s]=static_cast<unsigned char>(zone);
        put(0x16290+s*8,0x58000);put(0x16294+s*8,0x68000);
        put(0x16218+2*8,0xA0000);put(0x1621C+2*8,0xB0000);
        rotations.clear();
        assert(lagiAzelResolveCurrentMapContext(out));
        assert(out.subfield==int(zone>=15?15:zone));
        assert(out.x==(zone>=15?7:17));
        assert(out.y==(zone>=15?1015:1046));
        if(s==1)assert(out.heading==0x1234567 && rotations.empty());
        else {assert(out.heading==0x3456789);assert((rotations==std::vector<int>{0,1,2,3}));}
        ++cases;
    }
    field.m2E_currentSubFieldIndex=36;
    assert(!lagiAzelResolveCurrentMapContext(out) && out.field==-1);
    field.m2C_currentFieldIndex=1;field.m2E_currentSubFieldIndex=0;
    mainGameState.mode=1;assert(!lagiAzelResolveCurrentMapContext(out));mainGameState.mode=0;
    fieldPtr=nullptr;assert(!lagiAzelResolveCurrentMapContext(out));fieldPtr=&field;
    subfield.m338_pDragonTask=nullptr;assert(!lagiAzelResolveCurrentMapContext(out));subfield.m338_pDragonTask=&dragon;
    commonObject.m_dataSize=0xE850;assert(!lagiAzelResolveCurrentMapContext(out));commonObject.m_dataSize=unsigned(common.size());
    put(0x1000,0);assert(!lagiAzelResolveCurrentMapContext(out));put(0x1000,0x201100);
    gCommonFile=nullptr;assert(!lagiAzelResolveCurrentMapContext(out));gCommonFile=&commonObject;
    std::printf("PASS: live normal map context, %u special-field routes, native rotation/atan bindings and guards (host mocks)\n",cases);
}
