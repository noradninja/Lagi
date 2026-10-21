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
