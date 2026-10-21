# Cloud performance

The accepted cloud/sand device run shows render samples around 47-50 ms with
clouds and 18-24 ms without. Cloud-frame GPU completion waits are 36-38 ms,
versus roughly 11-13 ms outside the effect; CPU preparation is about 9-11 ms.
The nbg0Resolve field measures CPU submission, not GPU execution.

First isolated optimization: final high-resolution RGBA composition now uses
a separate single-texture-sample shader rather than a mode of the large native
tile/line/window decoder. Native-raster decode, additive/premultiplied blend
factors, linear filtering, alpha edges and authored state remain unchanged.
The loading overlay shares the small shader with premultiplied opacity.
No stochastic filtering is introduced. This also reduces resident shader
variant code. The hypothesis is reduced full-resolution register/instruction
pressure; only a fresh device log can establish the improvement.

Target remains ideal 25 ms/frame, maximum 33 ms/frame, with accepted pixels.

Name-entry regression: frontend layout 2 collided with the cloud resolve
selector 2 introduced with cloud support. Keyboard activation consequently
ran the cloud decoder using frontend uniforms and produced missing/flickering
layers. World cloud resolve now explicitly uses mode 8; frontend layouts 0,
1 and 2 retain their original title/keyboard routing. Obsolete display modes
were removed because resolved RGBA now has its own shader. This dispatch fix
does not change native cloud math or the independent display optimization.

The next device run confirms name entry restored and clouds improved to 20
FPS, but cloud render samples remain about 45 ms with 32-34 ms GPU waits.
The next isolated optimization fetches each aligned native line longword from
one packed RGBA texel rather than two BE16 helpers. A production-math fixture
compares all 65,536 signed high words with varied fractional bits, verifies
one packed fetch/no halfword reads, and retains unaligned/wrapped addressing.
No device timing improvement is claimed until the next run.

The following run confirms particle fade recovery but still reports 20 FPS:
cloud render samples are roughly 39-41 ms with 28-30 ms completion waits.
Next, native cloud resolve uses an independent fragment program calling the
same tested sampleSceneNbg0 implementation. Frontend title/keyboard decoding
is no longer compiled into the cloud program, and cloud state is no longer
part of the frontend shader. Raster size, blends, window/line math and output
remain unchanged. This removes unrelated code and register pressure without
reducing resolution or effect fidelity; actual speedup remains device-gated.

Device feedback finds no meaningful gain from program isolation. The next
change follows the established RBG0 coefficient-row route: CPU prepares native
line X/Y/zoom once per scanline into a 256x2 F32F32 texture (4096 bytes). It
does not rasterize cloud pixels. The GPU samples those row values instead of
repeating signed byte decode, interval/stride and fixed-point conversion for
each of 352 pixels per row. Tile addressing, palette decode, windows, native
raster and blending remain on the same GPU route. Existing frame completion
protects row-buffer reuse, as with the RBG0 row texture.

The host fixture runs both production shader variants and compares output for
the existing native addressing/scroll/window test cases. SceneNBG0 records
lineRows=1 and rowBytes=4096. Fresh Vita timing and visual confirmation remain
required; this is not proof that the 25/33 ms target is met.

The row-path device run reports unchanged 20 FPS but cloud render samples of
32.551, 33.868, 34.079, 32.961, 34.063, 33.526, 33.152 and 33.550 ms, with
19.9-24.1 ms GPU waits. This is improvement versus the prior 39-41 ms samples,
not acceptance of the deadline. The next change conservatively clips native
resolve work to 32-pixel tiles enclosing a single rectangular inside window.
The existing exact pixel window test stays in the shader. Line windows,
outside windows and multi-window cases retain full-raster fallback. No cloud
resolution, blending, palette or placement changes are made.
