# Garil enemy attack trace - 21 November 2026

Status: the single/horned formation attack path is now restored in source and
passes host/native-contract checks and a Vita package build; hardware acceptance
is pending. Other formation attack paths remain incomplete. This
checkpoint concerns attacks only. It does not change Lathum, passage cameras,
renderer behavior, authored story flags or the accepted player/death/reward flow.

## Evidence and reproduction

Original `BTL_A5.PRG` loads at `0x06054000`. The captured
`gash-encounter-wram.bin` loads at `0x06000000`. The following complete ranges
were compared byte for byte; all agree. SHA256 values cover the half-open ranges
listed, not the complete files.

| Range | SHA256 |
| --- | --- |
| `06054080..06054384` | `55179634420c92c0cc8a713d11230c62d0f0927c827a6e1344cc00d0d4ff52c8` |
| `0605B1A8..0605BBD0` | `c69d78491a44f4084973a47892e1183dce1f8a58c4e01e4beff36c5f7931d7cf` |
| `0605BE3C..0605C280` | `8b02d0158a491342db74302bc6b9941da4fec3d804fb338a29f86c5879c8abc2` |
| `060A2A62..060A2B2E` | `94ee70e96a986a0e05e7351fb2d7c4fca371c087f8240606f72c54cd32015179` |

The external tracing workspace is
`C:/Users/Noradninja/Documents/Codex/yabause-garil-pc`; its sibling
`azel-garil-pc/data/BTL_A5.PRG` supplies the original program. For bounded native
inspection, set `GARIL_DISASM_BASE=06000000` and run
`capture-build/Release/garil_saturn.exe --disasm gash-encounter-wram.bin START END`.
Keep instruction operands distinct from decimal annotations: immediate `0x4B`
is 75 frames, `0x2D` is 45 and `0x3C` is 60.

## Missing handoffs at the traced baseline

The baseline generated single-formation update reached state 3 without
selecting an attack. Upstream only checks `battleEngine_isPlayerTurnActive` and
breaks. States 5 through 8 are also absent. Simply adding selection would send
these states into the existing default cleanup arm.

Native state 3 (`0605B2FE`) waits for the player-turn helper to return false,
requires engine `m3CC->m8 == 1`, and requires formation byte `+F3 == 0`, then
calls the selector at `0605B83C`.

The selector chooses the lead pair (indices 0/1), ordinary actors (indices 2
onward), or an authored dragon-movement turn according to quadrant, dragon
flags, remaining actors and its mode counter. Do not replace this with a random
or always-front attack. It reads signed 32-bit death records at formation `+F4`,
the actor group at `+E0`, and the busy mask at `+F3`. Null/finished actors are
removed from the group. Formation bookkeeping at `0605B558` also manages
formation positions and the busy/selection fields; it must be recovered before
assuming those fields stay zero.

Selected living actors are readied through `060A2A62(group,index)`: OR actor
work-area flags with `0x08000000` and `0x04000000`. The selector then calls
`060619AA(1)` and `06061CAE(1)` and selects state 5. These addresses are the
literal values at `0605BB64/68`; do not infer callees from instruction labels
inside the literal pool.

| State | Native behavior |
| --- | --- |
| 5 | Wait for engine flag `0x2000`; display attack 7 if formation byte `+10D` is zero, otherwise 8, with arguments `0x23,0`; select state 6. |
| 6 | Postincrement counter; when its old signed value exceeds `0x2D`, reset it, call `060A2AB0(group)`, select state 7. |
| 7 | Postincrement counter; when old value exceeds `0x3C`, reset it, set engine `0x100`, select state 8. |
| 8 | Wait until the player-turn helper is false, return to state 3, reset engine `m3CC->m8/m0`, call `060A289E` (tail-call target `060818D0`). |

`060A2AB0` filters dead tasks, then only for actors with work-area flag
`0x04000000` clears that bit and ORs `0x02000000`. It returns whether any actor
transitioned. These are **actor work-area flags**, not task-header flags. Reuse
the existing typed formation actor traversal and typed actor receivers; never
cast a host work area to its Saturn byte offsets. The generic upstream
`formationCheckEntityFlag` currently reads task-header flags and the generic
attack writers remain stubs, so their names alone do not establish parity.

## Actor wind-up and release

Native single actor state 0 sees `0x04000000`, selects state 1 and creates a
wind-up quad task through `0605C51C`. Leads use hotpoint record offset `+C8`
and quad list `060AFE44`; other actors use the first record and list
`060AFAFC`. Each record's `+8` supplies the live position pointer. Preserve RNG
ordering for the formation byte increment following this creation.

The wind-up task uses resource slot 6. Its descriptor at `060B0048` supplies
update `0605C582` and draw `0605C650`. It follows the live hotpoint while its
parent actor state is 1, adds `0x2000` to Y/Z during update, advances animation,
and grows each authored scale from zero by `0x333` to `0x4000`. When the actor
leaves state 1 the task finishes. The draw writes these scales into its current
quad then uses the shared projected-particle draw. Borrowed hotpoint ownership
and mutable quad data need explicit host-safe lifetimes/copies.

Actor state 1 sees `0x02000000`, selects state 2 and calls projectile factory
`06054080(parent,hotpoint,initialVelocity,type)`, then plays sound `0x69`.
Leads pass type 3; others pass 0. Native `0605BFC6..0605C014` copies engine
scroll X/Y and explicitly subtracts engine scroll Z from its copied Z, giving
initial Z velocity zero. Source position uses the same lead/ordinary hotpoints.
Actor state 2 retains its existing postincrement wait of old counter > `0x4B`,
then clears `0x02000000` and returns to idle.

## Original homing projectile, not the Valley ballistic descriptor

`06054080` creates the projectile under the caller and binds resource slot 6.
It copies source and velocity, sets a start delay of `randomNumber() & 9`, a
signed lifetime of `0x4B` and an unset hit latch. Type 3 uses quad `060AFE44`,
damage **33** (`0x21`) and damage flags 8. Other types use quad `060AFAFC`,
damage **24** (`0x18`) and flags 0. It initializes the authored animated quad
without the Valley factory's random initial animation advance.

Update `06054160` first adds the engine auto-scroll to position and advances
the quad every frame. It postdecrements delay and returns if the old delay was
positive. Once active, it computes all three acceleration components from the
same pre-integration position/velocity:

```
acceleration[axis] = ((dragonPosition[axis] - position[axis]) >> 7)
                   - (velocity[axis] >> 3);
velocity += acceleration;
position += velocity;
```

It then calls collision/damage at `060542B4`. Finally it postdecrements lifetime
and finishes/clears draw when the **old** lifetime was negative. Preserve these
postdecrement boundaries and signed arithmetic.

Collision transforms projectile and dragon positions into view space, gives
the projectile radius `0x2000` and dragon radius `0x1000`, and calls shared
sphere collision `06064400`. That helper accepts touching spheres, computes
the component midpoint with signed division rounding toward zero, and uses
`06036038` to transform the point back out of view space. On collision,
`060542B4` calls `applyDamageToDragon` (`060642F8`) with the recovered damage,
collision output position, attack kind 2, zero impact vector and recovered
damage flags. It sets its hit latch and finishes/clears draw, preventing repeat
damage. Passing the dragon position or projectile velocity instead would not
match this call.

## Integration boundary and outstanding work

Reuse Lagi's shared model/hotpoint ownership, immutable quad descriptor cache,
projected-particle renderer and `applyDamageToDragon` pipeline. Extend shared
projectile support with the native homing motion/impact contract; do not route
these shots through the different A3 ballistic equation or guess damage.
Preserve the accepted Valley projectile behavior and the accepted A5 death,
reward and exit lifecycle. Upstream files remain untouched; generated adapters
must fail clearly if their source boundaries change.

This trace covers the single/horned formation. Nanyd, dual, variant and Lathum
attack producers/selection still require their own original-code audit. Before
activating any selector, restore its full selection/bookkeeping, engine handoff,
wind-up, release, damage and completion path. Source/build checks will not prove
Vita attack timing, visuals or battle progression; matching hardware acceptance
remains open.

## Single-formation implementation checkpoint

Lagi now restores the complete single/horned formation attack path through
its generated adapter: native turn gates and quadrant/status/death-dependent
selection, movement-only turns, shared attack camera 1 and attack-quadrant flag,
name display, readied-to-release actor flags, homing shot creation/damage,
completion signal and shared camera restoration. The established all-dead,
reward and field-return path is retained. Other A5 formations are unchanged.

The generated single-formation type extends a renamed copy of the upstream
formation definition with typed writable layouts. Three decoded original
position tables persist through layout switches for this encounter, including
ordered spacing updates and their RNG ordering. The overlay assets remain
unchanged. Native layout cycling also restores the original quadrant speed and
altitude values. Original records at +F4 remain typed-copied 32-bit death records;
work-area layout offsets are never applied to polymorphic host objects.

Charge and homing services are shared Lagi runtime functions. Charge frames
are task-private copies of cached authored quad descriptors, grow by 0x333 to
0x4000 and follow the world-space targetable position while the actor stays
in its charging state. Parent-child task retirement keeps borrowed state and
hotpoint storage alive; the targetable arrays remain in the parent task heap.
Native targetable +8 is m8, pointing at its world-space m10_position. This is
distinct from the model's view-space hotpoint array. Leaders use record 2 and
other actors use record 0. The existing m1BC_isLead2 member holds the role
passed by the constructor; it is used semantically, not as a native byte offset.

The shared homing service preserves original delay, arithmetic shifts, scroll,
75-frame lifetime with old-counter expiry, collision radii/contact position,
33/24 damage, damage kind 2, zero impact vector and flags 8/0. Its immutable
quad cache, projected-particle rendering and applyDamageToDragon service are
shared with established combat infrastructure. Valley ballistic shots retain
their original path. The native attack-name function itself uses a fixed
45-frame display duration; the recovered call's extra 0x23,0 arguments are not
used by that native function, so the shared implementation is retained.

Validation executes the generated actor stages, generated formation selection
and phases, shared charge/projectile runtime and typed actor traversal. Cases
cover all 64 death masks with both status policies; quadrant movement and
player-turn/busy gates; exact old-counter charge/recovery boundaries; null and
finished actors; separation of task-header and actor flags; layout persistence,
spacing order/thresholds/RNG/clamps; live world-space source selection; private
animated charge frames; signed motion, delay/expiry and one-hit contact damage.
Original BTL_A5.PRG anchors verify native role/record indices, quad pointers,
selector callees, timing, damage, flag masks and layout bounds. Existing model
initialization and Valley projectile regression cases pass. Full Vita compile,
link and VPK packaging pass. These are source/build checks, not device proof.

Next device check for this checkpoint: allow the single/horned formation to
take a turn, confirm charge/projectile/contact damage and return to player
control, defeat it and enter another encounter. Existing first-Nanyd, dual,
variant and Lathum producers/selection still need their own traces; do not
interpret this checkpoint as all Garil enemies attacking or full Garil parity.
