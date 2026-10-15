#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

struct LagiAzelMapDescriptor {
    std::uint32_t nativeAddress = 0;
    const char* mapFile = nullptr;
    const char* patternFile = nullptr;
    // Preserve original bytes +8..B; interpretation belongs to native task.
    std::array<std::uint8_t,4> presentation{};
};

struct LagiAzelMapContext {
    LagiAzelMapDescriptor descriptor{};
    int field = -1;
    int subfield = -1;
    std::int16_t x = 0, y = 0;
    std::int32_t heading = 0;
};

// Native 06016C54: live field/dragon context, including special-field zones.
// False means no native field map is available; never substitute radar data.
bool lagiAzelResolveCurrentMapContext(LagiAzelMapContext& out);
bool lagiAzelMapContextIsCurrent(const LagiAzelMapContext& context);
bool lagiAzelPresentMapContextPatterns(const LagiAzelMapContext& context,
    const std::uint8_t* first,std::size_t firstBytes,const std::uint8_t* second,std::size_t secondBytes);
unsigned lagiAzelDrawMapContextObjectMarkers(const LagiAzelMapContext& context,
    std::int16_t scrollX,std::int16_t scrollY,std::int16_t verticalOffset);
// Native 06016B6C/06016BAA. Text comes from COMMON, with the menu font and
// original message rectangle/cursor placement. Availability is not ownership.
void lagiAzelClearMapMessage();
bool lagiAzelPresentMapMessage(int selected,const std::array<std::int32_t,2>& availability);

struct s_workArea;
struct LagiAzelMapViewStatus {
    bool setupSucceeded=false;
    bool finished=false;
};
struct LagiAzelMapViewSource {
    LagiAzelMapContext context{};
    std::array<std::uint8_t,0x1000> patterns{};
    bool resourcesReady=false;
    std::shared_ptr<LagiAzelMapViewStatus> status;
};
// Original field-map view child 06017670/060174C8. Source is copied at Init;
// parent owns resource loading/restoration and retains status, not a raw child.
s_workArea* lagiAzelCreateFieldMapView(s_workArea* parent,const LagiAzelMapViewSource& source);

// Original 06017228: 32-pixel cells relative to the native backing-layer scroll.
inline std::array<std::int16_t,2> lagiAzelMapViewScrollOrigin(
    std::int16_t centerX,std::int16_t centerY,std::int16_t backingX,std::int16_t backingY)
{
    return {static_cast<std::int16_t>((int(centerX)-2)*32+backingX-0x60),
        static_cast<std::int16_t>((int(centerY)-2)*32+backingY-0x20)};
}

// Original 06016F90: marker viewport clipping in native coordinates.
inline bool lagiAzelMapMarkerScreenPosition(std::int32_t x,std::int32_t y,
    std::int16_t scrollX,std::int16_t scrollY,std::int16_t verticalOffset,
    std::array<std::int16_t,2>& out)
{
    const std::int64_t screenX=std::int64_t(x)-scrollX;
    const std::int64_t screenY=std::int64_t(y)-scrollY;
    if(screenX<0x60 || screenX>0x120 ||
        screenY<0x20-int(verticalOffset) || screenY>0xE0-int(verticalOffset)) return false;
    out={static_cast<std::int16_t>(screenX),static_cast<std::int16_t>(screenY)};
    return true;
}

// Native 060172B8: one 16x16 block half of a 32x32-word PNB plane.
// The caller supplies original map-ownership and exploration services.
inline bool lagiAzelConvertMapPatterns(const std::uint8_t* source,std::size_t sourceBytes,
    std::uint8_t* destination,std::size_t destinationBytes,bool mapOwned,
    unsigned firstY,bool (*explored)(void*,unsigned,unsigned),void* context)
{
    if(!source || !destination || sourceBytes<0x800 || destinationBytes<0x800 ||
        !explored || firstY<15) return false;
    for(unsigned row=0;row<16;++row) for(unsigned x=0;x<16;++x) {
        const bool visible=explored(context,x,firstY-row);
        for(unsigned dy=0;dy<2;++dy) for(unsigned dx=0;dx<2;++dx) {
            const unsigned offset=row*0x80+x*4+dy*0x40+dx*2;
            std::uint16_t pattern=0x0238;
            if(visible || mapOwned) {
                pattern=std::uint16_t((std::uint16_t(source[offset])<<8)|source[offset+1]);
                pattern=std::uint16_t(pattern+(visible?0x4000:0x7000));
            }
            destination[offset]=std::uint8_t(pattern>>8);
            destination[offset+1]=std::uint8_t(pattern);
        }
    }
    return true;
}

// Native 06017A68: task fields +0/+8/+C/+10/+1C. Actions identify the
// original service to run; this does not enable the unfinished constructor.
enum class LagiAzelMapAction {
    None, SetupSelection, SetupStandaloneMode1, InputSelection,
    InputMode1, InputMode2, CloseMenu, CloseStandalone, Finish
};
struct LagiAzelMapLifecycle {
    bool standalone=false;
    std::int32_t phase=0, countdown=16, mode=0, availabilitySlot0=0;
};
struct LagiAzelMapSelectionResult {
    enum Action {None,Cancel,Rejected,EnterMode1,EnterMode2,Moved} action=None;
    int sound=-1;
    int oldHighlight=-1,newHighlight=-1;
};
// Original 06017762..060178A2, native pressed/repeat word. Slot -1 is absent,
// 0 remains selectable but rejects confirm; slot 1 receives highlight updates.
inline LagiAzelMapSelectionResult lagiAzelMapSelect(
    std::int32_t& selected,const std::array<std::int32_t,2>& availability,
    std::uint16_t input)
{
    using Result=LagiAzelMapSelectionResult;
    if(input&1) return {Result::Cancel,1};
    if(selected<0 || selected>1) return {};
    if(input&6) {
        if(availability[selected]==0) return {Result::Rejected,5};
        return {selected==0?Result::EnterMode1:Result::EnterMode2,0};
    }
    if(!(input&0x30)) return {};
    const int old=selected;
    // Two-slot wrapping has the same result for up/down. Bound the native
    // skip loop defensively if malformed input marks both slots absent.
    int next=1-selected;
    if(availability[next]==-1) next=selected;
    if(availability[next]==-1) return {};
    selected=next;
    return {Result::Moved,10,availability[old]==1?old:-1,
        availability[next]==1?next:-1};
}
struct LagiAzelMapStep {
    LagiAzelMapAction action=LagiAzelMapAction::None;
    bool finished=false;
};
inline LagiAzelMapStep lagiAzelAdvanceMapLifecycle(LagiAzelMapLifecycle& state)
{
    switch(state.phase) {
    case 0:
        if (--state.countdown==0) ++state.phase;
        return {};
    case 1:
        ++state.phase;
        return {state.standalone && state.availabilitySlot0==1
            ? LagiAzelMapAction::SetupStandaloneMode1 : LagiAzelMapAction::SetupSelection,false};
    case 2:
        switch(state.mode) {
        case 0: return {LagiAzelMapAction::InputSelection,false}; // 06017762
        case 1: return {LagiAzelMapAction::InputMode1,false}; // 060178A4
        case 2: return {LagiAzelMapAction::InputMode2,false}; // 06017978
        default: return {};
        }
    case 3:
        if(state.standalone) return {LagiAzelMapAction::CloseStandalone,true};
        state.countdown=16;
        ++state.phase;
        // Native closing setup falls through to the first wait tick.
        --state.countdown;
        return {LagiAzelMapAction::CloseMenu,false};
    case 4:
        if(--state.countdown==0) return {LagiAzelMapAction::Finish,true};
        return {};
    default: return {};
    }
}

// Original 06016B04, using the native COMMON compressed tile layout.
bool lagiAzelSetupMapEntry(bool standalone);
// Native 06012554: current field's authored inventory item (COMMON A650).
bool lagiAzelCurrentFieldMapOwned();
unsigned lagiAzelDrawMapObjectMarkers(int subfield,unsigned subfieldCount,
    std::int16_t scrollX,std::int16_t scrollY,std::int16_t verticalOffset);
// Original 0601249C; caller supplies its authoritative subfield count.
bool lagiAzelMapCellExplored(int subfield,unsigned subfieldCount,int x,int y);
// Native 06017376..88: bind original exploration/ownership to both VRAM planes.
bool lagiAzelPresentMapPatterns(const std::uint8_t* first,std::size_t firstBytes,
    const std::uint8_t* second,std::size_t secondBytes,int subfield,unsigned subfieldCount);
// Original map-view child Delete 0601763C; parent-menu priority restoration.
void lagiAzelRestoreMapViewPriorities();
// Original 060173AA..06017406, using the native map-view scroll pair.
void lagiAzelSetupMapViewWindow(std::int16_t scrollX,std::int16_t scrollY);
// Native view setup copies the computed origin to NBG2's live scroll pair.
std::array<std::int16_t,2> lagiAzelSetMapViewCenter(std::int16_t x,std::int16_t y);

// Original 06016E64. Caller owns the pattern buffer and graphics rollback.
// Returns the native SCB VRAM offset; no generated or substitute map image.
bool lagiAzelLoadFieldMapResources(const LagiAzelMapDescriptor& descriptor,
    std::uint8_t* patterns, std::size_t capacity, std::uint32_t& graphicsVramOffset);

// Vita ownership around the native loader: retain exactly the overwritten
// graphics span and both pattern planes until all view children are gone.
// This is rollback/lifetime compatibility, not an alternative map producer.
class LagiAzelMapResources {
public:
    LagiAzelMapResources()=default;
    ~LagiAzelMapResources();
    LagiAzelMapResources(const LagiAzelMapResources&)=delete;
    LagiAzelMapResources& operator=(const LagiAzelMapResources&)=delete;
    bool load(const LagiAzelMapDescriptor& descriptor);
    void restore();
    bool ready() const {return ready_;}
    std::uint32_t graphicsVramOffset() const {return graphicsOffset_;}
    const std::array<std::uint8_t,0x1000>& patterns() const {return patterns_;}
private:
    std::array<std::uint8_t,0x1000> patterns_{};
    std::array<std::uint8_t,0x1000> planeBackup_{};
    std::vector<std::uint8_t> graphicsBackup_;
    std::uint32_t graphicsOffset_=0;
    bool active_=false,ready_=false;
};

// Original 06016D36..46: COMMON table 0020E848, 23 field pointers, then
// subfield pointers to 12-byte descriptors. Never infer a subfield count from
// adjacent COMMON data; the wrapper below requires the caller's real count.
// Returned strings borrow COMMON's lifetime. No disc resources are copied.
// For a slot already selected by native field/zone state. Arbitrary UI indices
// must use the count-checked wrapper below. Both readers bound image accesses.
inline bool lagiAzelReadFieldMapDescriptorSlot(const std::uint8_t* common,
    std::size_t bytes, std::uint32_t field, std::uint32_t subfield,
    LagiAzelMapDescriptor& out)
{
    out = {};
    constexpr std::uint32_t base=0x200000, table=0xE848;
    if (!common || field>=23 ||
        bytes<table || bytes-table<23*4) return false;
    const auto be32=[](const std::uint8_t* p) {
        return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|
            (std::uint32_t(p[2])<<8)|p[3];
    };
    const auto range=[&](std::uint32_t address,std::size_t length) {
        return address>=base && std::size_t(address-base)<=bytes &&
            length<=bytes-std::size_t(address-base);
    };
    const std::uint32_t subtable=be32(common+table+field*4);
    if (!range(subtable,4) || std::uint64_t(subfield)*4>bytes-(subtable-base)-4) return false;
    const std::uint32_t descriptor=be32(common+(subtable-base)+std::size_t(subfield)*4);
    if (!range(descriptor,12)) return false;
    const auto* record=common+(descriptor-base);
    const std::uint32_t map=be32(record), pattern=be32(record+4);
    if (!range(map,1) || !range(pattern,1) ||
        !std::memchr(common+(map-base),0,bytes-(map-base)) ||
        !std::memchr(common+(pattern-base),0,bytes-(pattern-base))) return false;
    out.nativeAddress=descriptor;
    out.mapFile=reinterpret_cast<const char*>(common+(map-base));
    out.patternFile=reinterpret_cast<const char*>(common+(pattern-base));
    std::copy(record+8,record+12,out.presentation.begin());
    return true;
}

inline bool lagiAzelReadFieldMapDescriptor(const std::uint8_t* common,
    std::size_t bytes, std::uint32_t field, std::uint32_t subfield,
    std::uint32_t subfieldCount, LagiAzelMapDescriptor& out)
{
    out = {};
    return subfield < subfieldCount && lagiAzelReadFieldMapDescriptorSlot(
        common,bytes,field,subfield,out);
}
