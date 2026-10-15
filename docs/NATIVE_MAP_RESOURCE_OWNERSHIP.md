# Map resource ownership and rollback

`LagiAzelMapResources` is the Vita lifetime wrapper around the existing
original-derived `lagiAzelLoadFieldMapResources`. It does not replace the
loader, generate a map, alter ownership flags or substitute a texture.

## Parent integration

After native Map backing setup, the current parent loads the resolved
descriptor's real SCB and PNB. It retains the source PNB halves and graphics
backup for its lifetime. `[FieldMapResources]` reports readiness and the
native graphics destination. Missing context does not attempt file loading.
The parent explicitly restores resources before restoring priorities/status
backing. Its member destructor is an additional idempotent cleanup path.

Native SCB destination selection is unchanged: sizes strictly greater than
4000hex use69000; smaller/equal use6C000. PNB capacity is1000hex, including
the native short-PNB second-half fill. Source padding retains the existing
loader behavior. No filename-based size/position/palette override is used.

## Ownership guarantees

Before loading, retain the exact SCB destination span and both pattern planes
at70000..70FFF. Both snapshots happen before any file write: large graphics
loads can overlap those planes. On load failure, restore both VRAM spans and
the previous CPU pattern buffer, clear readiness/destination and permit retry.
This also covers loaders that return failure after partially writing data.

An already active owner refuses another load without damaging its current
lease. Normal restore or destruction clears readiness and releases the backup
allocation. Repeated cleanup is inert. Unrelated VRAM addresses are not rolled
back. Successful source patterns remain owned until the parent is destroyed;
a future view copies them through its existing source contract.

The native file index/payload is assumed stable during a load, as in the
original loader. This is not a transactional filesystem or protection against
external replacement of mounted game files mid-operation. VRAM restoration
is captured by the existing VDP2 snapshot/change-detection path, not a new
GPU resource allocation or texture-filter change.

## Evidence and remaining work

`tests/map_resources_owner_test.ps1` compiles the actual ownership methods
and actual native loader body. Host fixtures exercise22 successful/overlap/
partial-failure combinations, forced destructor cleanup, exact-span restore,
reentry refusal, retry and missing/oversized input rejection. Partial failures
write data before returning an error. The parent load/cleanup ordering has
source guards; actual parent/child scheduling is not executed by this fixture.
Existing loader and view-task tests also pass. Vita syntax checks cover the
owner, parent and field-view child.

No VPK, real packaged-file load, on-device rollback, rendered Map image or
timing result is claimed. The top-level Map page still shows a placeholder;
its native selection/mode/cursor lifecycle and resource-to-view handoff are
not connected. Both field/world choices must be preserved, and the viewer's
context/count contract must be satisfied without inventing subfield counts.
This resource checkpoint is not completion of Map or the broader menu goal.
