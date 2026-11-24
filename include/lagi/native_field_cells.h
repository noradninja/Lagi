#pragma once
#include <cstdint>
#include <map>

namespace lagi {
// Overlay identity is part of the key: native addresses are reused by overlays.
// Registrations survive field re-entry; rebinding replaces, rather than appends.
template<class Cell, class Placement> class NativeFieldCellFactories {
public:
    using Factory = void(*)(Cell*, Placement&, int);
    void bind(const void* owner, std::uint32_t address, Factory factory) {
        factories[owner][address] = factory;
    }
    bool dispatch(const void* owner, std::uint32_t address,
                  Cell* cell, Placement& placement, int memoryArea) const {
        const auto overlay = factories.find(owner);
        if (overlay == factories.end()) return false;
        const auto found = overlay->second.find(address);
        if (found == overlay->second.end() || !found->second) return false;
        found->second(cell, placement, memoryArea);
        return true;
    }
private:
    std::map<const void*, std::map<std::uint32_t, Factory>> factories;
};
}

struct sSaturnMemoryFile;
struct s_visdibilityCellTask;
struct s_DataTable2Sub0;
using LagiNativeFieldCellFactory = void(*)(s_visdibilityCellTask*, s_DataTable2Sub0&, int);
void lagiRegisterNativeFieldCellFactory(const sSaturnMemoryFile*, unsigned, LagiNativeFieldCellFactory);
bool lagiDispatchNativeFieldCellFactory(s_visdibilityCellTask*, s_DataTable2Sub0&, int);
