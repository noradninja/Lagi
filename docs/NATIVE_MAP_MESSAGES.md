# Original Map selection messages

`lagiAzelPresentMapMessage` and `lagiAzelClearMapMessage` implement MAIN
06016B6C/06016BAA. They use the native rectangle `(8,38hex,1Chex,4)`, clear
it, select `graphicEngineStatus.m40AC.fontIndex` through `setActiveFont`,
then reset the string flags and draw only when the original helper has text.
The clear-only call retains the original string flags. The font call at
06025104 was previously described as a palette operation; it is a font switch.

COMMON provides the four authored strings at20D9B9,20D9CF,20D9DE,20D9EE.
Field selection draws its primary message, repositions to the rectangle's
left edge/two rows below its top, and adds the unavailable-map message only
when availability is not1. World selection chooses its allowed/blocked
message without that cursor reposition. The service validates COMMON bounds
and string termination before drawing. Bad/absent input clears stale text
instead of reading an arbitrary address or inserting substitute English.

The current entry shell now uses this field-area prompt once after entry,
instead of `FIELD MAP` and inventory-based acquired/not-acquired labels.
Whether the area's descriptor exists is distinct from whether the player
owns its map item; ownership still controls the original explored/unexplored
pattern conversion. Item flags/collection are not modified by this change.

`tests/map_message_test.ps1` executes the actual message service with mocked
font/text calls, checking both selections/availability branches, exact
rectangle, call order, cursor reposition, clear-only flags and malformed
resource rejection. It also reads Disc1 COMMON without modification and
resolves all four real strings. Native source and parent Vita syntax pass.
These checks do not prove glyphs, font switching or placement on a Vita.

This does not complete the native parent selector. The current parent still
does not expose the native two-choice cursor/input/mode flow or construct
the view child. Both field/world choices and their original availability
and confirmation/transition services remain required. The full Map image,
menu acceptance and broader in-game menu goal are still unfinished.
