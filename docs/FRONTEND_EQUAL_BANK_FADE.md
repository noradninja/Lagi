# Frontend identical-bank fades

The device trace for D5 name entry shows CLOFEN=007F and CLOFSL=0010 while
both A and B contain the same fading RGB values. RBG0 selects B; particles
select A. The former shared-fade predicate accepted only uniform selectors,
so this case faded RBG0 separately and left particles visible too early.

All enabled layers with identical A/B RGB values now share one post-composite
offset even if their selectors differ. If bank values differ, the existing
per-layer behavior remains unchanged. The shared pass remains after text and
sprite composition, avoiding duplicate fades. Production-helper tests span
the signed-nine-bit fade range and reject partial enable masks or unequal
R/G/B banks. Visual synchronization still requires device confirmation.
