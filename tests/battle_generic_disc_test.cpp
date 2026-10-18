#include "lagi/battle_startup.h"
#include "lagi/battle_animation_states.h"
#include <array>
#include <vector>
#include <string>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <set>
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
using s8=std::int8_t;
using s16=std::int16_t;using s32=std::int32_t;using fixedPoint=s32;
using quadColor=std::array<u16,4>;
using sVec3_FP=std::array<fixedPoint,3>;
static FILE* disc=nullptr;
static std::vector<u8> extent(u32 lba,u32 size){
    if(size>0x1000000)throw std::runtime_error("Unexpected extent size");
    std::vector<u8> bytes(size);
    for(size_t at=0;at<size;at+=2048){
        const auto count=std::min<size_t>(2048,size-at);
        if(_fseeki64(disc,(std::uint64_t(lba)+at/2048)*2352+16,SEEK_SET) ||
            fread(bytes.data()+at,1,count,disc)!=count)throw std::runtime_error("Disc read failed");
    }
    return bytes;
}
static u32 little(const u8* p){return u32(p[0])|u32(p[1])<<8|u32(p[2])<<16|u32(p[3])<<24;}
static std::vector<u8> readProgram(const char* filename){
    const auto pvd=extent(16,2048);
    if(std::string(reinterpret_cast<const char*>(pvd.data()+1),5)!="CD001")throw std::runtime_error("Expected MODE1/2352 ISO9660");
    const auto root=extent(little(pvd.data()+158),little(pvd.data()+166));
    for(size_t at=0;at<root.size();){
        const auto size=root[at];
        if(!size){at=(at/2048+1)*2048;continue;}
        if(size<34 || at+size>root.size() || 33u+root[at+32]>size)throw std::runtime_error("Malformed directory");
        std::string name(reinterpret_cast<const char*>(root.data()+at+33),root[at+32]);
        name=name.substr(0,name.find(';'));
        if(name==filename)return extent(little(root.data()+at+2),little(root.data()+at+10));
        at+=size;
    }
    throw std::runtime_error("BTL_A3.PRG not found");
}
struct sSaturnMemoryFile;
struct sSaturnPtr{
    sSaturnMemoryFile* file;union {u32 address;u32 m_offset;};
    sSaturnPtr operator+(u32 n)const{return {file,address+n};}
    sSaturnPtr& operator+=(u32 n){address+=n;return *this;}
    bool isNull()const{return address==0;}
};
struct sSaturnMemoryFile{
    std::vector<u8> storage;u8* m_data;u32 m_dataSize;u32 m_base=0x06054000;
    explicit sSaturnMemoryFile(const char* name):storage(readProgram(name)),m_data(storage.data()),m_dataSize(static_cast<u32>(storage.size())){}
    sSaturnPtr getSaturnPtr(u32 address){return {this,address};}
};
static unsigned reads=0;static u32 highest=0;
static u32 read(sSaturnPtr p,unsigned count){
    if(p.address<p.file->m_base || std::uint64_t(p.address-p.file->m_base)+count>p.file->m_dataSize)
        throw std::runtime_error("Native generic table read outside BTL_A3.PRG");
    ++reads;highest=std::max(highest,p.address+count);u32 value=0;
    for(unsigned i=0;i<count;++i)value=value<<8|p.file->m_data[p.address-p.file->m_base+i];
    return value;
}
u8 readSaturnU8(sSaturnPtr p){return static_cast<u8>(read(p,1));}
s8 readSaturnS8(sSaturnPtr p){return static_cast<s8>(read(p,1));}
u16 readSaturnU16(sSaturnPtr p){return static_cast<u16>(read(p,2));}
s16 readSaturnS16(sSaturnPtr p){return static_cast<s16>(read(p,2));}
u32 readSaturnU32(sSaturnPtr p){return read(p,4);}
s32 readSaturnS32(sSaturnPtr p){return static_cast<s32>(read(p,4));}
sVec3_FP readSaturnVec3(sSaturnPtr p){return {readSaturnS32(p),readSaturnS32(p+4),readSaturnS32(p+8)};}
fixedPoint readSaturnFP(sSaturnPtr p){return static_cast<s32>(read(p,4));}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {p.file,read(p,4)};}
std::string readSaturnString(sSaturnPtr p){
    std::string result;if(!p.address)return result;
    for(u8 c=readSaturnU8(p);c;c=readSaturnU8(p)){result.push_back(char(c));p+=1;}
    return result;
}
#include "battle/battleGenericData.h"
#include "quad.inc"
#include "generic.inc"
#include "entry.inc"
#include "dispatch.inc"
struct sHotpointBundle {};
// Model/hotspot loading is outside this bounded formation-data test.
sHotpointBundle* readRiderDefinitionSub(sSaturnPtr){return nullptr;}
#include "formation_types.inc"
#include "formation.inc"
struct DigitCommand{
    u16 m0_CMDCTRL=0,m4_CMDPMOD=0,m6_CMDCOLR=0,m8_CMDSRCA=0,mA_CMDSIZE=0,m1C_CMDGRA=0;
    s16 mC_CMDXA=0,mE_CMDYA=0,m14_CMDXC=0,m16_CMDYC=0;
};
struct DigitPacket{u16 m4_bucketTypes=0;DigitCommand* m6_vdp1EA=nullptr;};
struct DigitContext{
    DigitCommand* m0_currentVdp1WriteEA;
    std::vector<quadColor> m14[1];std::vector<quadColor>::iterator m10;
    DigitPacket* m20_pCurrentVdp1Packet;unsigned m1C=0,mC=0;
};
struct {DigitContext m14_vdp1Context[1];}graphicEngineStatus;
#include "digits.inc"
namespace charge_fixture {
struct sGaugeIncreaseEffect{s32 m10_decayRate,m60_alpha,m5C_rotationY,m58_rotationX,m4C_colorSetIndex;sVec3_FP* m50_pPosition;sVec3_FP m64_offset;};
struct {int m28[2]={1,2};}cameraProperties2;
struct Allocation{u16 m4_vdp1Memory=0x3100;}allocation;
struct Bundle{Allocation* m4_vd1Allocation=&allocation;}bundle;
struct Entry{Bundle* mC_fileBundle=&bundle;};std::array<Entry,1>dramAllocatorEnd;
s32 MTH_Mul(s32 a,s32 b){return s32((std::int64_t(a)*b)>>16);}
int stackDepth=0;void pushCurrentMatrix(){++stackDepth;}void popMatrix(){--stackDepth;}
void translateCurrentMatrix(sVec3_FP*){}void rotateCurrentMatrixShiftedY(s32){}void rotateCurrentMatrixShiftedX(s32){}
void transformAndAddVecByCurrentMatrix(const sVec3_FP* a,sVec3_FP* b){for(int i=0;i<3;++i)(*b)[i]=(*a)[i]+10;}
void transformAndAddVec(const sVec3_FP& a,sVec3_FP& b,int matrix){assert(matrix==1);for(int i=0;i<3;++i)b[i]=a[i]+20;}
std::array<sVec3_FP,2> capturedPoints;std::array<s32,2> capturedWidths;quadColor capturedColors;unsigned calls=0;
void displayRaySegment_2Width(std::array<sVec3_FP,2>& p,std::array<s32,2>& w,u16 address,s16 size,u16 color,const quadColor* colors,s32 mode){
    assert(stackDepth==0 && address==0x3DF8 && size==0x320 && color==0x5FF0 && mode==8);
    capturedPoints=p;capturedWidths=w;capturedColors=*colors;++calls;
}
#include "charge.inc"
void check(){
    for(unsigned color=0;color<3;++color){
        sVec3_FP position={100,200,300};sGaugeIncreaseEffect task{0xCCC,0x10000,123,456,s32(color),&position,{1,2,3}};
        sGaugeIncreaseEffect_Draw(&task);
        if(capturedPoints[0]!=position || capturedPoints[1]!=sVec3_FP{31,32,33} || capturedWidths!=std::array<s32,2>{0,0xCCC})
            throw std::runtime_error("Charge endpoints/taper differ from native draw");
        for(unsigned i=0;i<4;++i)if(capturedColors[i]!=readSaturnU16(g_BTL_GenericData->getSaturnPtr(0x060ACE54+color*8+i*2)))
            throw std::runtime_error("Charge Gouraud table differs");
    }
    const u32 anchors[][2]={{0x06085352,0x2EF0},{0x06085354,0x0320},{0x06085356,0x0CF8},{0x0608538C,0x060ACE54},{0x06085390,0x0602F610}};
    for(const auto& a:anchors)if(read(g_BTL_GenericData->getSaturnPtr(a[0]),a[1]>0xFFFF?4:2)!=a[1])throw std::runtime_error("Native charge draw anchor changed");
    std::puts("PASS: actual generated charge draw supplies two initialized endpoints, native zero-width taper, sprite offsets/size/mode and runtime Gouraud table; original draw literals verified. Mocked transforms, not GPU acceptance.");
}
}
struct sBattleManager{s16 m2_currentBattleOverlayId,m4_previousBattleOverlayId,m6_subBattleId,m8_previousSubBattleId;};
#include "remap.inc"
void checkA3Dispatch(){
    sSaturnMemoryFile common("COMMON.DAT");common.m_base=0x00200000;
    struct Setup{std::string prg;unsigned count;};std::vector<Setup> setups;
    for(unsigned i=0;i<27;++i){
        const auto entry=common.getSaturnPtr(0x2005DC+i*0x14);
        setups.push_back({readSaturnString(readSaturnEA(entry+4)),readSaturnU32(entry+0xC)});
    }
    std::set<int> subbattles;
    for(unsigned i=0;i<setups.size();++i)for(unsigned sub=0;sub<setups[i].count;++sub){
        sBattleManager manager{};manager.m2_currentBattleOverlayId=s16(i);manager.m6_subBattleId=s16(sub);
        remapBattleOverlayId(&manager);
        if(manager.m2_currentBattleOverlayId<0 || unsigned(manager.m2_currentBattleOverlayId)>=setups.size())throw std::runtime_error("Native remap outside COMMON table");
        if(setups[manager.m2_currentBattleOverlayId].prg=="BTL_A3.PRG")subbattles.insert(manager.m6_subBattleId);
    }
    unsigned formations=0;
    for(const int sub:subbattles){
        auto entry=sOverlayBattleEntry::read(g_BTL_GenericData->getSaturnPtr(0x060A5688+sub*0x20));
        if(std::find(std::begin(mapFunctions),std::end(mapFunctions),entry.m8_mapFunction.address)==std::end(mapFunctions)){
            std::fprintf(stderr,"sub=%d missing map dispatch %08X\n",sub,entry.m8_mapFunction.address);throw std::runtime_error("Unimplemented authored A3 map");
        }
        read(entry.m4_positionData+0x59,1); // engine consumes the native 0x5A-byte block
        auto formation=entry.m0_formationTable;
        for(u32 address=readSaturnU32(formation);address;address=readSaturnU32(formation)){
            if(std::find(std::begin(formationFunctions),std::end(formationFunctions),address)==std::end(formationFunctions)){
                std::fprintf(stderr,"sub=%d missing formation dispatch %08X\n",sub,address);throw std::runtime_error("Unimplemented authored A3 formation");
            }
            readSaturnU32(formation+4);readSaturnU32(formation+8);++formations;formation+=0xC;
        }
    }
    std::printf("PASS: native COMMON remapper reaches %zu A3 subbattles and %u formation records; all map/formation functions match native dispatch cases. Data coverage only, not callback execution.\n",subbattles.size(),formations);
}
void checkA3Animations(){
    // Signed sentinel, recursive discovery, cycles and the largest s8 index.
    std::array<std::int8_t,128*0x24+4> synthetic;
    synthetic.fill(-1);
    const auto origin=g_BTL_GenericData->getSaturnPtr(1);
    const auto null=g_BTL_GenericData->getSaturnPtr(0);
    const auto next=[&](sSaturnPtr p){return synthetic.at(p.address-1);};
    if(lagi::battle_animation_state_count(origin,null,null,next)!=1)
        throw std::runtime_error("-1 animation sentinel became an index");
    synthetic[0x18]=1;synthetic[0x24+0x18]=2;synthetic[2*0x24+0x18]=0;
    if(lagi::battle_animation_state_count(origin,null,null,next)!=3)
        throw std::runtime_error("Animation transition closure/cycle failed");
    synthetic.fill(-1);synthetic[127*0x24+0x18]=127;
    if(lagi::battle_animation_state_count(origin,origin+127*0x24,null,next)!=128)
        throw std::runtime_error("Beam override/highest signed index failed");
    const u32 parameters[]={0x060A8A88,0x060A8A04,0x060A7C08,0x060A7CF8};
    const int expected[]={1,2,1,3};
    for(unsigned i=0;i<4;++i){
        const auto params=g_BTL_GenericData->getSaturnPtr(parameters[i]);
        const auto table=readSaturnEA(params+0x1C);
        const int count=lagi::battle_animation_state_count(table,
            readSaturnEA(params+0x20),readSaturnEA(params+0x24),
            [](sSaturnPtr p){return static_cast<std::int8_t>(read(p,1));});
        if(count!=expected[i]){
            std::fprintf(stderr,"params=%08X states=%d expected=%d\n",parameters[i],count,expected[i]);
            throw std::runtime_error("Unexpected authored A3 animation reachability");
        }
        for(int state=0;state<count;++state){
            const auto record=table+state*0x24;
            const auto effect=readSaturnEA(record);
            if(!effect.isNull() && std::find(std::begin(effectFunctions),std::end(effectFunctions),effect.address)==std::end(effectFunctions)){
                std::fprintf(stderr,"params=%08X state=%d missing effect factory=%08X\n",parameters[i],state,effect.address);
                throw std::runtime_error("Authored animation effect missing from live A3 dispatcher");
            }
            // Exercise all native record fields, including pointers, within the file.
            read(record+0x20,2);
            for(unsigned q=0;q<4;++q){
                const auto command=readSaturnEA(record+4+q*4);
                if(!command.isNull())read(command+0xA,1);
            }
        }
    }
    for(const u32 address:{0x060A8300u,0x060A84C0u,0x060A87B4u}){
        const auto frames=initVdp1Quad(g_BTL_GenericData->getSaturnPtr(address));
        if(frames.empty() || frames.back().m0_isLast!=1)
            throw std::runtime_error("Native ambient particle descriptor missing its terminator");
    }
    const u32 emitterAnchors[][2]={{0x06056AF4,0xD513},{0x06056AFE,0xE604},
        {0x06056B04,0x7308},{0x060569BC,0xC807},{0x06056B1C,0x20216E80},
        {0x06056B24,0x20217E80},{0x06056B22,0xA000},
        {0x06056B12,0x00C0},{0x06056B38,0x00010000}};
    for(const auto& anchor:emitterAnchors)
        if(read(g_BTL_GenericData->getSaturnPtr(anchor[0]),anchor[1]>0xFFFF?4:2)!=anchor[1])
            throw std::runtime_error("Original native ambient emitter anchor changed");
    std::puts("PASS: A3 authored animation counts 1,2,1,3; beam-hit transitions, subsequent state 2 and 0x24-stride records are readable. Not combat execution.");
}
void checkA3Formations(){
    // Original code anchors: escape/death use +2C, hit sound +2D, death
    // sound +2E. Parameter owner is loaded from the enemy's +CC pointer.
    const u32 anchors[][2]={
        {0x060763CE,0xE12C},{0x060763F2,0x001C},{0x060763F6,0x8804},
        {0x0607645E,0x00CC},{0x06076C16,0xE12E},{0x06076C1E,0x001C},
        {0x06076C5A,0xE12C},{0x06076C60,0x001C},
        {0x0607750E,0xE12D},{0x06077524,0x044C},{0x06077528,0x644C}
    };
    for(const auto& anchor:anchors)
        if(readSaturnU16(g_BTL_GenericData->getSaturnPtr(anchor[0]))!=anchor[1])
            throw std::runtime_error("Original A3 enemy parameter instruction anchor changed");
    const u32 roots[]={0x060A8AE8,0x060A8AC4,0x060A7D34,0x060A7D58};
    unsigned types=0,states=0;
    for(const u32 address:roots){
        const auto root=g_BTL_GenericData->getSaturnPtr(address);
        auto* formation=readUrchinFormation(root);
        for(unsigned type=0;type<3;++type){
            auto* params=formation->m4_perTypeParams[type];
            const auto original=readSaturnEA(root+4+type*4);
            if(original.isNull()){if(params)throw std::runtime_error("Unexpected enemy parameters");continue;}
            if(!params || params->m2C_deathMode!=readSaturnU8(original+0x2C) ||
                params->m2D_hitSound!=readSaturnU8(original+0x2D) ||
                params->m2E_deathSound!=readSaturnU8(original+0x2E))
                throw std::runtime_error("Enemy death/sound parameters differ from native bytes");
            for(unsigned q=0;q<4;++q)if(params->m28[q]!=readSaturnS8(original+0x28+q))
                throw std::runtime_error("Quadrant color table changed");
            const auto table=readSaturnEA(original+0x1C);
            const int count=lagi::battle_animation_state_count(table,readSaturnEA(original+0x20),
                readSaturnEA(original+0x24),[](sSaturnPtr p){return readSaturnS8(p);});
            if(params->m1C.size()!=count)throw std::runtime_error("Actual formation reader omitted animation states");
            for(unsigned state=0;state<params->m1C.size();++state){
                auto& parsed=params->m1C[state];const auto record=table+state*0x24;
                if(!parsed.m0.isNull() && std::find(std::begin(effectFunctions),std::end(effectFunctions),parsed.m0.address)==std::end(effectFunctions)){
                    std::fprintf(stderr,"formation=%08X type=%u state=%u missing effect=%08X\n",address,type,state,parsed.m0.address);
                    throw std::runtime_error("Parsed enemy animation effect missing from live dispatcher");
                }
                if(parsed.m1C_animationOffset!=readSaturnU16(record+0x1C) ||
                    parsed.m20_timerValue!=readSaturnU16(record+0x20))
                    throw std::runtime_error("Actual animation reader did not advance source record");
                for(auto* command:parsed.m4)delete command;
                ++states;
            }
            for(auto* response:{params->m20_beamHitCounterData,params->m24_beamHitAttackData}){
                if(response){for(auto* command:response->m4)delete command;delete response;}
            }
            delete params;++types;
        }
        for(auto* positions:formation->m10_formationSubData)delete positions;
        delete formation;
    }
    std::printf("PASS: actual generated formation parser read %u enemy parameter sets / %u animation records; all non-null animation effect factories are registered; death mode/sounds and quadrant colors match separate native fields. Hotspot/model loading mocked.\n",types,states);
}
void checkDamageDigits(){
    const u32 anchors[][2]={{0x0605BDB8,0x1001},{0x0605BDBC,0x048C},
        {0x0605BDC0,0x060B1F7C},{0x0605BDC8,0x060B1F04},{0x0605C11C,0x060B1FF4}};
    for(const auto& a:anchors)if(read(g_BTL_GenericData->getSaturnPtr(a[0]),a[1]>0xFFFF?4:2)!=a[1])
        throw std::runtime_error("Original floating-number instruction/table anchor changed");
    for(int mode=0;mode<2;++mode)for(unsigned color=0;color<4;++color){
        DigitCommand commands[10];DigitPacket packets[10];
        auto& c=graphicEngineStatus.m14_vdp1Context[0];
        c.m0_currentVdp1WriteEA=commands;c.m20_pCurrentVdp1Packet=packets;
        c.m14[0].resize(16);c.m10=c.m14[0].begin();c.m1C=c.mC=0;
        lagiDrawBattleDamageDigits("0123456789",-40,30,8,mode?16:11,0x3100,color,mode);
        if(c.mC!=10 || c.m1C!=10 || c.m10!=c.m14[0].begin()+10)throw std::runtime_error("Digit packet counters differ");
        for(unsigned d=0;d<10;++d){
            const auto sprite=g_BTL_GenericData->getSaturnPtr((mode?0x060B1F04:0x060B1F7C)+d*12);
            auto& cmd=commands[d];
            if(cmd.m0_CMDCTRL!=0x1001 || cmd.m4_CMDPMOD!=0x48C ||
                cmd.m8_CMDSRCA!=u16(readSaturnU16(sprite+2)+0x3100) ||
                cmd.m6_CMDCOLR!=u16(readSaturnU16(sprite+6)+0x3100) ||
                cmd.mA_CMDSIZE!=readSaturnU16(sprite+8) || cmd.mC_CMDXA!=-40+int(d)*8 ||
                cmd.mE_CMDYA!=-30 || cmd.m14_CMDXC!=cmd.mC_CMDXA+7 ||
                cmd.m16_CMDYC!=(mode?16:11)-31 || cmd.m1C_CMDGRA!=d || packets[d].m6_vdp1EA!=&cmd)
                throw std::runtime_error("Floating-digit geometry/texture/packet differs from native emitter");
            for(unsigned v=0;v<4;++v)if(c.m14[0][d][v]!=readSaturnU16(g_BTL_GenericData->getSaturnPtr(0x060B1FF4+color*8+v*2)))
                throw std::runtime_error("Floating-digit Gouraud colors differ from authored table");
        }
    }
    puts("PASS: production floating-digit emitter covers all digits, both animation modes and all four authored Gouraud color sets with original instruction/table anchors. Mocked VDP1 context, not GPU pixels.");
}
int main(int argc,char** argv){
    if(argc!=2 || fopen_s(&disc,argv[1],"rb") || !disc)return 2;
    try{
        BTL_GenericData::staticInit();
        if(!g_BTL_GenericData || !reads || g_BTL_GenericData->m_dataSize<lagi::battle_generic_minimum_bytes)throw std::runtime_error("Generic data did not initialize");
        if(BTL_GenericData::m_0x60abef4_animatedQuads.size()!=9 || BTL_GenericData::m_0x60b0658_animatedQuad.empty())throw std::runtime_error("Missing native animation tables");
        std::printf("PASS: real BTL_A3.PRG bytes=%u; actual generic constructor/shared quad reader completed %u bounded reads through %08X; laser nodes=%d,%d,%d; animation groups=%zu. Read-only host data test, not battle execution.\n",
            g_BTL_GenericData->m_dataSize,reads,highest,BTL_GenericData::mLaserData.m20_numLaserNodes,
            BTL_GenericData::mBoltLaserData0.m20_numLaserNodes,BTL_GenericData::mBoltLaserData1.m20_numLaserNodes,
            BTL_GenericData::m_0x60abef4_animatedQuads.size());
        checkA3Dispatch();
        checkA3Animations();
        checkA3Formations();
        checkDamageDigits();
        charge_fixture::check();
        delete g_BTL_GenericData;fclose(disc);return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());fclose(disc);return 1;}
}
