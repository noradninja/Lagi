#pragma once

#include <cstddef>

// Vita persistence boundary for Azel's native save/load tasks. Azel owns the
// save payload, checksum, slot selection, menus, and game-state transitions;
// Lagi only supplies durable storage for the original three-slot interface.
extern "C" {

int lagiAzelSaveDeviceAvailable(int deviceId);
int lagiAzelSaveFreeSlots(int deviceId);
int lagiAzelSaveHasAnyValidSlot();
int lagiAzelSaveRead(
    int deviceId,
    const char* fileName,
    void* destination,
    std::size_t size);
int lagiAzelSaveWrite(
    int deviceId,
    const char* fileName,
    const void* source,
    std::size_t size);
void lagiAzelSaveMarkInGameLoad();
int lagiAzelSaveConsumeInGameLoad();

}
