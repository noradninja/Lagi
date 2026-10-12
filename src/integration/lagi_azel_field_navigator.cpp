// Native FLD_A3 navigator services: 06073120, 06072F3C, 06072E78, 06072EE8.
// Retain Azel's real updateMultiChoice task, cursor, input and box lifecycle.
#include "lagi/lagi_azel_upstream_prelude.h"
#include "field/fieldRadar.h"
#include "field/field_a3/o_fld_a3.h"
#include "kernel/dialogTask.h"
#include <memory>
#include <unordered_map>

void dispatchTutorialMultiChoiceSub1();

const char* lagiFieldNavigatorText(u32 address)
{
    if (!address) return "";
    assert(gFLD_A3 && address >= gFLD_A3->m_base &&
           address - gFLD_A3->m_base < gFLD_A3->m_dataSize);
    return reinterpret_cast<const char*>(getSaturnPtr(gFLD_A3->getSaturnPtr(address)));
}

u32 lagiFieldNavigatorDefaultTextEA()
{
    // Native reads script slot 0 at 06095A28, rounds its payload up to a word,
    // then reads its first string EA. Do not substitute an English literal.
    const u32 script = readSaturnU32(gFLD_A3->getSaturnPtr(0x06095a28));
    return readSaturnU32(gFLD_A3->getSaturnPtr((script + 3u) & ~3u));
}

namespace {
struct NavigatorStringView {
    sSaturnMemoryFile file;
    std::vector<u8> bytes;
};
// String-table data must survive until the native dialog's Delete callback,
// including parent-driven teardown. readSaturnEA retains its source m_file;
// copy/rebase strings into the same view as its big-endian pointer table.
std::unordered_map<const void*, std::unique_ptr<NavigatorStringView>> views;

bool available(u32 bit)
{
    if (bit >= 1000) bit -= 0x236;
    assert((bit >> 3) < sizeof(mainGameState.bitField));
    return (mainGameState.bitField[bit >> 3] & bitMasks[bit & 7]) != 0;
}

s32 destinationIndex(s_FieldRadar* radar, s32 filteredIndex)
{
    if (filteredIndex < 0) return -1;
    auto* words = reinterpret_cast<const u32*>(radar->m1C_encounterList);
    for (s32 i = 1; i < radar->m14_encounterCount; ++i) {
        if (!available(words[i * 5 + 1])) continue;
        if (filteredIndex-- == 0) return i;
    }
    return 0; // Trailing native deactivate entry.
}
}

s32 lagiOpenFieldNavigatorDialog()
{
    auto* field = getFieldTaskPtr()->m8_pSubFieldData;
    auto* script = field->m34C_ptrToE;
    auto* radar = field->m33C_fieldRadar;
    auto* choice = script->m44_multiChoiceData;
    if (!choice || script->m3C_multichoiceTask) return 0;
    const s32 count = radar->m24_filteredChoiceCount;
    assert(count > 0 && count <= 127 && radar->m20_choiceArray);
    assert(choice->m4_currentChoice >= 0 && choice->m4_currentChoice < count);
    auto view = std::make_unique<NavigatorStringView>();
    view->bytes.resize(static_cast<size_t>(count) * 4);
    for (s32 i = 0; i < count; ++i) {
        const char* text = lagiFieldNavigatorText(static_cast<u32>(radar->m20_choiceArray[i]));
        const u32 offset = static_cast<u32>(view->bytes.size());
        for (unsigned b = 0; b < 4; ++b)
            view->bytes[static_cast<size_t>(i) * 4 + b] = static_cast<u8>(offset >> (24 - b * 8));
        const auto length = std::strlen(text);
        view->bytes.insert(view->bytes.end(), text, text + length + 1);
    }
    view->file.m_data = view->bytes.data();
    view->file.m_dataSize = static_cast<u32>(view->bytes.size());
    view->file.m_base = 0;
    auto* dialog = updateMultiChoice(script, &script->m3C_multichoiceTask,
        &choice->m4_currentChoice, count, view->file.getSaturnPtr(0),
        choice->m0_choiceTable, choice->m4_currentChoice);
    views.emplace(dialog, std::move(view));
    return 0; // Script discards the original open routine's result.
}

void lagiReleaseFieldNavigatorDialog(const void* task)
{
    views.erase(task);
}

s32 lagiFinishFieldNavigatorDialog()
{
    auto* field = getFieldTaskPtr()->m8_pSubFieldData;
    auto* script = field->m34C_ptrToE;
    auto* radar = field->m33C_fieldRadar;
    if (script->m3C_multichoiceTask) return 0;
    auto* choice = script->m44_multiChoiceData;
    assert(choice);
    const s32 selected = destinationIndex(radar, choice->m4_currentChoice);
    if (selected >= 0) {
        const auto* words = reinterpret_cast<const u32*>(radar->m1C_encounterList);
        setGlobalStringTableEntry(2, selected ? lagiFieldNavigatorText(words[selected * 5]) : "");
        radar->m18_currentSelection = selected;
    }
    dispatchTutorialMultiChoiceSub1(); // Native 0607933E frees choice styles/data.
    freeHeapForTask(radar, radar->m20_choiceArray);
    radar->m20_choiceArray = nullptr;
    return 1;
}
