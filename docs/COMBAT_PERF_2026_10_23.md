# Full-run combat performance follow-up

User accepted the divide crash and repeated-tunnel audio fixes on hardware.
The next log contains two Patergo (sub 7) encounters with sampled renderer
medians 33.49/30.54 ms and complete presentation windows 19.98–23.90 / 22.55–29.97
FPS. Other ordinary encounters are approximately 29.5–29.97 FPS. These are
sampled renderer spans, not independent CPU/GPU costs or whole-frame averages.

Battle entry cold renders take 96–158 ms; RBG preparation alone takes 56–60 ms.
The decoded-page cache expands three 512x512 pages on resource invalidation.
The first correction memoizes exact repeated pattern names within each page
rebuild, copying already decoded rows rather than repeating per-dot decoding.
Dependencies are collected before reuse; name equality includes flips/palette,
and all producer-domain/generation invalidation rules remain intact. No effect,
rotation, blending, or native timing is skipped. Direct-map collisions miss
safely. Native Azel pixel-oracle and production cache-service tests pass.

The pre-boss cinematic has a sampled 34.51-ms task spike and 31.25-ms renderer
span containing a static cache rebuild. Task/render spans may overlap. This
is a separate hitch candidate, not evidence of sustained cloud GPU overload.

Hardware validation remains required for each change: battle entry fades,
both Patergo variants including Regenerate, the pre-boss cinematic, name
entry, water, and clouds. A build or pixel test is not a timing acceptance.

## Shared dynamic model positions

The adapter now retains each expanded vertex's first native position-index
occurrence. During a submission the renderer may copy XYZ from an earlier
quad already transformed under the same current matrix/projection/billboard
basis. RGBA is retained from the current face; normals, lighting and material
records are not shared. Nothing is reused across actors or frames. Missing
topology and noncanonical geometry retain the accepted path. This applies to
all native models, not Patergo IDs. The production topology adapter test and
bitwise append-versus-baseline fixture pass for repeated appends, lights,
billboards, offsets, blend ordering, face colors and fallback layouts.
