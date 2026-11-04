# Explicit device-test profile. Apply with cmake -C cmake/world_garil_profile.cmake.
# FORCE is intentional here: opting into this profile also repairs existing caches.
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
set(LAGI_FULLRES ON CACHE BOOL "" FORCE)
set(LAGI_BATTLE_NATIVE_BUILD ON CACHE BOOL "" FORCE)
set(LAGI_BATTLE_EXECUTE_A3 ON CACHE BOOL "" FORCE)
set(LAGI_WORLD_MAP_BRINGUP ON CACHE BOOL "" FORCE)
# Required for the accepted native water color-calculation path.
set(LAGI_VDP2_SPRITE_COMPOSITE ON CACHE BOOL "" FORCE)
# Serialized GPU timing is diagnostic, not an ordinary performance build.
set(LAGI_GPU_PASS_TIMING OFF CACHE BOOL "" FORCE)
set(LAGI_FIELD_MOTION_TRACE OFF CACHE BOOL "" FORCE)
