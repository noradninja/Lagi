#include "lagi/disc_image.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
namespace lagi::disc {
std::string g_imagePath="accepted-disc4.bin";
std::uint32_t g_rootExtent=45,g_rootSize=67,g_physicalSectorSize=2352,g_userDataOffset=24,g_trackStartLba=321;
bool g_mounted=true;int g_selectedDisc=4;
bool mountOk=true,openOk=true,readOk=true;
unsigned fileBytes=16,closes=0,mounts=0;
bool mount_disc(int number){++mounts;g_imagePath="temporary.bin";g_rootExtent=100;g_rootSize=200;g_physicalSectorSize=2048;g_userDataOffset=0;g_trackStartLba=0;g_mounted=mountOk;g_selectedDisc=number;return mountOk;}
bool open_file(const char* name,FileHandle& handle){assert(!std::strcmp(name,"ASCII.CGZ"));handle.size=fileBytes;handle.native=openOk?&handle:nullptr;return openOk;}
void close_file(FileHandle& handle){assert(handle.native && g_imagePath=="temporary.bin");++closes;handle.native=nullptr;}
bool read_file_at(FileHandle&,std::uint64_t offset,void* dst,std::size_t bytes){assert(offset==0 && bytes==fileBytes);std::memset(dst,0xAB,bytes);return readOk;}
#include "scoped_disc.inc"
static void unchanged(){assert(g_imagePath=="accepted-disc4.bin" && g_rootExtent==45 && g_rootSize==67 && g_physicalSectorSize==2352 && g_userDataOffset==24 && g_trackStartLba==321 && g_mounted && g_selectedDisc==4);}
}
int main(){using namespace lagi::disc;std::vector<std::uint8_t> out;
assert(read_file_from_disc(1,"ASCII.CGZ",out,64) && out.size()==16 && out[0]==0xAB);unchanged();assert(closes==1);
mountOk=false;assert(!read_file_from_disc(2,"ASCII.CGZ",out,64) && out.empty());unchanged();assert(closes==1);mountOk=true;
openOk=false;assert(!read_file_from_disc(2,"ASCII.CGZ",out,64));unchanged();assert(closes==1);openOk=true;
readOk=false;assert(!read_file_from_disc(2,"ASCII.CGZ",out,64) && out.empty());unchanged();assert(closes==2);readOk=true;
fileBytes=65;assert(!read_file_from_disc(2,"ASCII.CGZ",out,64) && out.empty());unchanged();assert(closes==3);
fileBytes=0;assert(!read_file_from_disc(2,"ASCII.CGZ",out,64));unchanged();assert(closes==4);
const auto before=mounts;assert(!read_file_from_disc(0,"ASCII.CGZ",out,64));assert(!read_file_from_disc(5,"ASCII.CGZ",out,64));assert(!read_file_from_disc(1,nullptr,out,64));assert(!read_file_from_disc(1,"ASCII.CGZ",out,0));assert(before==mounts);unchanged();
g_imagePath.clear();g_mounted=false;g_selectedDisc=0;fileBytes=16;
assert(read_file_from_disc(3,"ASCII.CGZ",out,64));assert(!g_mounted && g_selectedDisc==0 && g_imagePath.empty());
std::puts("PASS: production scoped disc read restores all eight mount fields on success/failure, enforces bounds and closes opened handles");}
