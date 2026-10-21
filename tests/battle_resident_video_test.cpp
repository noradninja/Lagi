#include "lagi/resident_video.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <unordered_map>
#include <cstdlib>
#undef assert
#define assert(expression) do { if(!(expression)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#expression);std::exit(1);} } while(false)
using u8=std::uint8_t;using u16=std::uint16_t;
std::array<u8,0x80000> vdp1{},vdp2{};std::array<u8,0x1000> cram{};
u8* getVdp1Pointer(unsigned a){return vdp1.data()+a-0x25C00000;}
u8* getVdp2Vram(unsigned a){return vdp2.data()+a;}
u8* getVdp2Cram(unsigned a){return cram.data()+a;}
int selectedDisc=1,notifications=0,reads=0;std::string failing;
std::map<std::string,std::vector<u8>> files;
namespace lagi::disc {
int selected_disc(){return selectedDisc;}
bool read_file(const char* name,std::vector<u8>& data){
    ++reads;if(!name || failing==name)return false;
    const auto found=files.find(name);if(found==files.end())return false;
    data=found->second;return true;
}}
namespace lagi::azel_bridge {void notify_native_texture_write(){++notifications;}}
namespace lagi::platform::logging {template<class... T>void writef(const char*,T...) {}}
struct sSaturnPtr{const char* p=nullptr;bool isNull()const{return !p;}};
std::string readSaturnString(sSaturnPtr p){return p.p;}
struct{int m0=0;}fileInfoStruct;
struct s_fileBundle {
    u8* raw;explicit s_fileBundle(u8* p):raw(p){}
    void setPatchFilePointers(u16){}~s_fileBundle(){delete[] raw;}
};
#include "resident.inc"
#include "file_service.inc"
using namespace lagi::resident_video;
void load(const char* name,u8* dest){assert(loadFile(name,dest,0)==0);}
int main(){
    files["A.CGB"]={1,2,3,4,5,6,7,8,9,10};files["B.CGB"]={21,22,23};
    files["BG.SCB"]={31,32,33,34};files["PAL.BIN"]={41,42};
    load("A.CGB",vdp1.data()+100);load("B.CGB",vdp1.data()+104);
    load("BG.SCB",vdp2.data()+500);load("PAL.BIN",cram.data()+10);
    static const auto expected1=vdp1,expected2=vdp2;const auto expectedC=cram;
    u8 heap[10]{};load("A.CGB",heap); // never retain a temporary heap address
    s_fileBundle* bundle=nullptr;assert(loadFile("A.CGB",&bundle,17)==0);delete bundle;
    assert(loadFile("MISSING",vdp1.data()+100,0)<0);
    Snapshot saved;capture(saved);assert(saved.spans.size()==5);
    const auto before=notifications;
    assert(!restore(saved) && notifications==before);
    vdp1.fill(0);vdp2.fill(0);cram.fill(0);
    load("B.CGB",vdp1.data()+100); // battle changes current registry, not snapshot
    const auto beforeRestore=notifications;
    failing="BG.SCB";assert(!prepare(saved));
    assert(vdp1[100]==21 && vdp2[500]==0 && cram[10]==0);
    failing.clear();const int start=reads;assert(prepare(saved));assert(reads-start==4);
    const int staged=reads;assert(prepare(saved) && reads==staged);
    Snapshot malformed=saved;malformed.spans.back().length=0x100001;
    assert(!restore(malformed) && notifications==beforeRestore && vdp1[100]==21);
    assert(restore(saved) && notifications==beforeRestore+1);
    assert(vdp1==expected1 && vdp2==expected2 && cram==expectedC);
    Snapshot restored;capture(restored);assert(restored.spans.size()==5);
    // A short/replaced file must not become a silent partial copy.
    files["A.CGB"].pop_back();assert(!prepare(restored));files["A.CGB"].push_back(10);
    assert(prepare(restored));
    // Nested byte backups restore provenance only for their owned spans.
    int outer=0,inner=0;
    save_scope(&outer,{{Space::vdp1,102,6},{Space::vdp1,104,2}});
    load("A.CGB",vdp1.data()+102);
    save_scope(&inner,{{Space::vdp1,104,3}});
    load("B.CGB",vdp1.data()+104);restore_scope(&inner);
    load("B.CGB",vdp2.data()+600); // unrelated write must survive scope return
    restore_scope(&outer);restore_scope(&outer);
    Snapshot scoped;capture(scoped);assert(prepare(scoped));
    for(size_t i=0;i<scoped.spans.size();++i)for(size_t j=i+1;j<scoped.spans.size();++j){
        const auto& a=scoped.spans[i];const auto& b=scoped.spans[j];
        assert(a.space!=b.space || a.destination+a.length<=b.destination || b.destination+b.length<=a.destination);
    }
    vdp1.fill(0);assert(restore(scoped));
    for(int i=102;i<108;++i)assert(vdp1[i]==expected1[i]);
    assert(vdp2[600]==21 && vdp1[108]==7 && vdp1[111]==10);
    // Disc switches invalidate both staged snapshots and active ownership.
    ++selectedDisc;assert(!restore(scoped) && !prepare(restored));
    Snapshot empty;capture(empty);assert(empty.spans.empty());
    --selectedDisc;capture(empty);assert(empty.spans.empty());
    puts("PASS: production raw-load tracking, partial overlaps/source offsets, staged retry/size validation, nested menu ownership, heap/bundle exclusion, disc changes and publication invalidation; mocked I/O");
}
