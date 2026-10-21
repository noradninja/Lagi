#include <map>
// The BTL_A3_2 data object is a process-lifetime singleton. Native particle
// tasks borrow immutable animation tables from that overlay. Keep the same
// stable ownership in the C++ adapter instead of replacing one shared vector.
static const std::vector<sVdp1Quad>& lagiBattleParticleFrames(sSaturnPtr source)
{
    static std::map<sSaturnMemoryFile*,std::map<u32,std::vector<sVdp1Quad>>> tables;
    auto& fileTables=tables[source.m_file];
    auto found=fileTables.find(source.m_offset);
    if(found==fileTables.end())
        found=fileTables.emplace(source.m_offset,initVdp1Quad(source)).first;
    return found->second;
}
