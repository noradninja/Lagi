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
