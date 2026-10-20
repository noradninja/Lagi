# Combat stick input

The battle input adapter was guarded by `LAGI_BATTLE_BUILD_EXECUTE_A3`, but
that definition was supplied only to `lagi_runtime.cpp`. Consequently all
battles compiled with stick-to-digital translation disabled. Supply the same
build definitions to the input bridge. Recognize native story battle mode 4
as well as the field encounter manager's running state 3.

The existing half-travel threshold and combined D-pad/stick press-edge handling
remain unchanged. Azel continues to own movement and repeat timing; flight
analog axes are unchanged. Compile/fixture checks are not Vita acceptance.
