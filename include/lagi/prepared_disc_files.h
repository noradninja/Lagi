#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace lagi::disc {
// A preparation transaction retains bytes, not pointers into the outgoing
// overlay. Normal loaders take each file once, under the same disc/name key.
class PreparedFiles {
    struct Entry { std::string name; std::vector<std::uint8_t> bytes; };
    int disc_ = -1;
    std::vector<Entry> entries_;
    static std::string key(const char* name) {
        std::string result = name ? name : "";
        for (auto& c : result) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        return result;
    }
public:
    void clear() { std::vector<Entry>().swap(entries_); disc_ = -1; }
    template<class Reader>
    bool prepare(int disc, const std::vector<std::string>& names, Reader read) {
        clear();
        std::vector<Entry> staged;
        for (const auto& name : names) {
            const auto normalized = key(name.c_str());
            if (normalized.empty()) return false;
            bool duplicate = false;
            for (const auto& entry : staged) duplicate |= entry.name == normalized;
            if (duplicate) continue;
            Entry entry{normalized, {}};
            if (!read(name.c_str(), entry.bytes)) return false;
            staged.push_back(std::move(entry));
        }
        entries_ = std::move(staged);
        disc_ = disc;
        return true;
    }
    bool take(int disc, const char* name, std::vector<std::uint8_t>& out) {
        if (disc != disc_) { clear(); return false; }
        const auto normalized = key(name);
        for (auto it = entries_.begin(); it != entries_.end(); ++it) {
            if (it->name != normalized) continue;
            out = std::move(it->bytes);
            entries_.erase(it);
            if (entries_.empty()) clear();
            return true;
        }
        return false;
    }
};

bool prepare_native_files(const std::vector<std::string>& names);
void clear_prepared_native_files();
}
