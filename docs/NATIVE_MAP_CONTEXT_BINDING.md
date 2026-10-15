# Map view context binding

The original field-view child stores a native selected map slot, not a
subfield-count field. The first Vita view adapter unnecessarily required a
count from its parent. That requirement is removed rather than inventing a
count from adjacent COMMON pointers or passing `selected+1` as metadata.

`LagiAzelMapViewSource` now carries the resolved native context, authored
patterns, readiness and completion object. Init checks that the live native
resolver still selects the same field, map slot and descriptor address.
Position/heading are intentionally not compared: the original constructor
captures those values once for the player marker.

Context-bound pattern and object-marker entry points perform the same live
validation before calling the existing native conversion/traversal. A stale
field, changed selected slot/descriptor, or missing current native context
rejects pattern publication and marker emission. A stale Init finishes
without setting up a viewport. Its normal Delete/completion cleanup remains.

The legacy count-checked entry points remain available for arbitrary indexed
callers and retain their count rejection. Their private shared slot consumers
now also bound the COMMON root, selected-slot word, exploration record,
rectangle and marker record before dereference. There is no new game flag,
exploration model, map image or inventory rule.

Host tests execute actual production binding/traversal/conversion, checking
matching context, stale field/slot/descriptor, absent resolver state,
no-write rejection and malformed EA hierarchy. The actual view task fixture
now uses the context-bound interface and tests stale Init/cleanup as well as
its reveal/pan/close and player-arrow behavior. Native service/view syntax
checks pass. Resolver and task services are mocked at fixture boundaries;
there is no real task scheduling, VPK, GPU or hardware acceptance claim.

This removes an artificial integration dependency. It does not connect the
unfinished native parent selector, cursor and field/world mode transitions
to the view factory. The Map page and broader in-game menu goal remain open.
