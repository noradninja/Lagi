# Native menu canvas boundary

User captures show save/load animation sprites and a Dragon ability-screen
shape outside the centered4:3 menu area. Existing layers/sprites use that
aspect correction, but the render target itself has wider side borders.

Neptune now draws two opaque black GPU side masks after the final menu glyph
plane, using the same4:3/output-aspect bounds. This is a presentation boundary,
not a change to Azel's positions, animation, sprite textures or draw priorities.
It covers all menu components, including3D preview fragments outside the canvas.
Only an active menu ID or save layout enables it. Gameplay/radar and ordinary
Cinepak presentation retain their existing wider composition.

Dedicated eight-vertex/twelve-index storage avoids overwriting queued UI/text/
cinematic-bar draws and is released with renderer resources. One GPU draw emits
both borders; no CPU pixel copying, new render target or extra framebuffer pass
is added. Depth testing/writes are disabled for the final masks.

Host execution checks actual canvas bounds for960x544,480x272,640x384,640x480
and portrait targets. Source checks confirm the menu gate, cleanup and placement
after glyphs. Menu-model ordering and pickup-composition regressions pass;
renderer Vita syntax passes. No VPK build, GPU execution or hardware acceptance
is claimed. Device gate: save/load slide transitions and Dragon screen must
keep all visible content inside the same4:3 boundary, while gameplay and FMV
remain unchanged. Dragon centering and animation speed are separate issues.
