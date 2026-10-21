# Boot presentation transitions

The CPU loading scanout stays visible while Neptune initializes and prepares
the immutable disc-selector textures. The ready selector crossfades from a
snapshot of that loading frame over 30 VBlanks using a premultiplied GPU overlay.
Steady selection continues to use immutable textures, without CPU composition.

After selection, the existing 30-VBlank fade reaches black. The black GXM
scanout retains display ownership during native initialization, until Azel
presents the opening Cinepak. It must not return to the CPU loading buffer.

`tests/launcher_transition_test.ps1` checks source contracts and opacity
endpoints; it is not a device visual test. Validate on Vita: loading remains
visible during preparation, crossfades to the selector, then fades to black
without loading text or stale pixels before the opening movie.
