#pragma once
#include <cstdint>

// Native 06016004/0601604E resource ownership boundary. The caller owns the
// returned DRAM block until restore; the Books task itself is not ported yet.
std::uint8_t* lagiAzelBooksBackupGraphics();
void lagiAzelBooksRestoreGraphics(std::uint8_t* backup);
