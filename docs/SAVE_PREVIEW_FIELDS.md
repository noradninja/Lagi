# Native save-slot preview fields

The original US executable's 060191C4 slot routine prints player name at X+2,
Y+1 and dragon name at X+2,Y+3. For X<24 it prints time at X+20,Y+1 and rider
level at X+20,Y+3, with palette C000. Formats at 060435C0/060435CC are exactly
`%2d:%02d` and `LV %2d`. The generated-source adapter restores these fields.

Mini-data +4 is the saved play-frame count, copied from raw save offset 030C
by 06026DF8, not the device number as the reconstructed source assumed. It maps
to gameStats.m3C_frameCounter; no on-disk save structures are changed. Native
time divides frames by 3600 to get minutes, caps at 5999, then splits hours
and minutes. Rider level is the saved zero-based level plus one. Dragon name
remains empty if its native named flag is unset.

The separate dragon-rank sprite routine 060192C2 is restored from disassembly:
it draws dragonLevel descriptors (not dragonLevel+1), each 8 bytes from COMMON
0020F734, with native position and palette 0760. Helper 06015BFC emits PMOD 0080,
matching drawMenuSprite, not the 8-bit drawMenuSprite2 helper. Valid level range
is 0..7; malformed negative/out-of-range values draw no marks. Rider `LV` is
distinct from these dragon-rank marks. Hardware appearance remains unverified.

Validation: patched native source passes Vita compiler syntax checks, with
time boundary/cap tests. No package build or device visual validation here.
User confirms in-game save/load stations function correctly on hardware;
preview display, cancellation cleanup and other menus retain separate gates.
