# Pickup notification composition

User capture shows collection particles and radial rays covering the pickup
box, icon and text. The former gameplay composition drew all foreground VDP1
commands after NBG1 tiles and NBG3 glyphs, including depth-tested world effects.

The renderer now partitions commands into background, world effects and
screen overlays. World effects draw after geometry but before NBG1 backing,
cinematic bars and NBG3 text; screen overlays remain late. Classification uses
existing producer world-depth metadata, not item names, texture IDs or guessed
screen rectangles. Radar commands remain screen overlays. Menus do not enable
world-depth classification and retain their existing order. All passes share
the sprite slot counter so queued vertex storage is not overwritten.

Actual production pass predicate executes on host for every background/world
combination, checking exactly one selected pass. Source-order checks cover
effects -> tiles -> glyphs -> overlays. Renderer Vita syntax passes. No VPK
build or Vita screenshot is claimed. Hardware gate is the full pickup panel
above particles/rays, with existing geometry occlusion and radar unchanged.
