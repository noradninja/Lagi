# Shared native solid-polygon mesh material

Goal 7 dependency for spark-ring world polygons. No extern/Azel edits.

Native A7 spark-ring polygon helper emits PMOD05C0: mesh bit0100 is independent
of half-transparency low bits. The GXM command4/6 solid polygon/line branch
previously selected only opaque versus half-transparent color programs and
ignored mesh. Textured/Gouraud sprite mesh already had separate shaders.

The shared solid-material selector now handles all four combinations using
only PMOD. Mesh gets a dedicated fragment program that discards alternating
destination pixels and returns the existing interpolated color/alpha unchanged.
Mesh+half uses the established native blend setup, including reserved RGB
alpha ownership when the VDP1/VDP2 compositor is enabled. Opaque non-mesh
gauges, ordinary color shaders, native world-depth metadata, geometry, packet
order, textured materials and fades are unchanged. No guessed particle texture,
source-ID exception, fullscreen pass, new surface or per-frame shader creation.

Registration/opaque+blended creation happen at existing GXM initialization;
release/unregister use existing teardown ownership. Creation failures remain
visible initialization failures, not silently opaque mesh substitutes.

The host fixture compiles the production material selector under both VDP2
compositor configurations and checks every65536 PMOD value. The actual shared
coverage predicate executes over960x544 pixels at three subpixel phases and
negative boundary coordinates; exactly half the pixels are discarded, matching
the existing textured-mesh parity. The actual Vita fragment compiler/reflection
confirms no samplers or uniforms. Binding/lifetime guards and existing textured
mesh regression tests pass; the full Vita VPK is built separately.

These checks do not prove device appearance or unchanged frame times. Spark
draw callbacks and the remaining A7 services still need integration before
Tutorial2 device acceptance. Other native solid-mesh uses also need visual
regression coverage; ordinary HUD polygons must remain unchanged.
