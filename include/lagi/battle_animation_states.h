#pragma once

namespace lagi {
// Native animation records are indexed at stride 0x24. There is no count
// field: follow signed next-state indices from state zero and beam overrides.
// -1 means retain the current state, not another record.
template<class Pointer, class ReadNext>
int battle_animation_state_count(Pointer table, Pointer counter, Pointer attack,
                                 ReadNext readNext)
{
    int highest = 0;
    const auto visit = [&](Pointer record) {
        for (int quadrant = 0; quadrant < 4; ++quadrant) {
            const int next = readNext(record + 0x18 + quadrant);
            if (next > highest) highest = next;
        }
    };
    if (!counter.isNull()) visit(counter);
    if (!attack.isNull()) visit(attack);
    for (int state = 0; state <= highest; ++state) visit(table + state * 0x24);
    return highest + 1;
}
}
