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

## First-Nanyd original-code trace

Native 0605CD10 update still calls selection at 0605D1F4 from its state 3
only after the shared player-turn gate is false and engine m3CC->m8 is 1.
Front quadrant 0 chooses an authored random left/right dragon movement. Side
quadrants 1/3 select ordinary actor rushes on two turns, then a movement turn.
Rear quadrant 2 starts a coordinated formation attack and switches the native
position table according to the formation variant. States 5/6 release actor
flags and signal completion after old counter >15; states 7/8 handle the
coordinated route and old counter >60. Recover their actor behavior before
activating these states. Accepted queen-loss Scatter remains state 9.

The actor's native +158 pointer is the formation's entity group (+E0), not
the formation root or a static data-table address. This is confirmed by the
original factory caller passing formation+E0 in r5 and constructor 0605E266
storing that argument. Thus native group+EC is formation+1CC (mapping table),
and native group+14 is formation+F4 (an 18-position formation history). These are
semantic bindings to named host fields, never host pointer arithmetic.

Actor rush preparation 0605EC0C stores the targetable pointer at +164, reads
its world-position pointer at targetable+8, computes target minus actor
position and calls the original shared look-at calculation. It seeds velocity
from yaw cosine/sine with coefficient -0x4A12, and pitch sine with +0x4A12,
then resets the actor counter. It is actor motion, not a shot factory.

Rush update 0605ECF0 recomputes the directional velocity, consumes independent
random perturbations in X,Z,Y order (random&255 minus128), advances the actor's
+90 offset and resolves world position from the mapping-selected writable
formation-history position plus that offset. Native collision uses actor radius 0x2000
and target radius 0x3000. A successful contact plays sound 0x67, applies damage
from actor+17C with attack kind 1, dragon world position, zero impact vector and
flags 0x0800, then returns completion. It does not substitute the collision
midpoint for that authored damage-position argument. A failed contact uses an
old-counter >0x45 timeout; if the queen-loss byte at group+10 is set, it retires
the actor and increments group dead count without a kill reward. This matters
when the queen dies while a survivor is already charging.

The existing 309bed0 contact geometry is now exposed as a shared world-space
sphere service for both shots and actor rushes. Homing shots retain radii
0x2000/0x1000 and their existing damage protocol; Nanyd callers will retain
their different radii and damage arguments. Host Valley/homing/charge regression
checks and production Vita object compilation pass for that refactor. The following implementation checkpoint connects these services to the Nanyd
formation and actor states. Device acceptance remains pending.

### Nanyd motion foundation

0605D1B2 writes the current formation position to history slot 0, then copies
slots 16 down to 0 into slots 17 down to 1. Thus slots 0/1 both contain the
current head, and slot 17 has a 16-update delay. This is not a conventional
FIFO push. The original 20-actor map at 060B0F04 is
0,3,3,3,3,6,6,6,6,6,9,9,9,9,12,12,12,12,12,15.

The native position routine 0605EAA4 adds the actor's authored formation-table
offset, orbit displacement and Scatter offset, then calls shared positional
spring 060A2450 around the history point selected by that map. It also calls
shared angular spring 060A262A around the formation rotation. Both use spring
0xFF and damping 0xFFF, signed-28-bit differences and cleared acceleration;
angular current wraps to 28 bits. These match the existing shared
stepNativeFormationSpring/stepNativeFormationAngularSpring services. The port's
quarter-step interpolation and per-actor auto-scroll addition are replaced by
these shared recurrences in the current uncommitted Nanyd work.

The adapter uses named position/rotation/velocity state and copies the declared
history/rotation data buffers through typed vectors. Live Y velocity and X/Y
angular velocity stay synchronized with the fields subsequently used by death
motion. Scatter signals, retirement, death notification and rewards are not
reimplemented. This motion foundation is required before wiring actor rushes
and the coordinated formation-table switch.

nanyd_motion_test.py executes the production motion/trail functions over
12,000 actor frames using the original 20- and 10-actor tables and map. It checks
history against an independent time-index oracle, native spring field binding,
rotation, offsets and live death-field synchronization. PRG anchors verify trail
count/order and spring constants. Existing independent scalar-SH spring tests
pass 897,775 position-spring, 859,375 angular-spring and 50,625 interpolation
cases. Production Nanyd actor and formation Vita objects compile. Full Nanyd
attack-state integration, packaging and device verification remain outstanding.

## Connected Nanyd attack implementation checkpoint

Nanyd formation state 3 now dispatches the original quadrant-dependent policy:
front movement left/right, two side rush turns followed by movement, or the
rear coordinated route. Shared typed formation traversal readies the first
three live actors for rushes or all live actors for coordination. Task-header
validity and actor work-area flags remain distinct. Readied 0x04000000 flags
transition to release 0x02000000 through the same helper used by single enemies.
The normal/coordinated tag 0x08000000 selects the native actor state family.

Native coordinated setup 0606196C matches the existing generated Valley
arachnoth_startDragonEvade function: direction 3, mode A, both interpolator
rates 60, m38E cleared. Nanyd calls that existing helper. The 20-actor route
switches to table 060B0E14 and the 10-actor route to 060B0F90. Both restore
060B0D24 at completion, exactly as the native update does. There is no guessed
table offset, replacement battle engine or forced progression flag.

Actor states 1/2 wait for release while advancing the model twice per update;
state 1 also uses native facing. State 3 advances the original local rush
offset, checks the shared contact service and applies native damage kind 1
(7 ordinary / 23 lead, flags 0x0800). It returns to idle on contact or a
non-queen-loss timeout. After queen loss, its timeout retires the meter and
actor without reward bits and updates formation dead count. State 4 follows
the coordinated formation with a one-hit latch, damage kind 2 (authored 2/4
ordinary or 14 lead, flags 0x0800), and old-counter >90 completion. It clears
the coordinated tag and latch when done. Damage checks continue in all alive
actor states. Existing death fall, Scatter signals, ordinary rewards and field
return are retained. Meter retirement without defeat is now one shared helper,
used by both accepted Scatter entry and the restored rush-timeout path.

Formation states 5/6 release rush actors and signal completion on old counter
>15; states 7/8 release the coordinated group and signal on old counter >60.
They then use the existing movement/turn handoff. The accepted queen-loss
barrier remains in front of state dispatch: it cannot replace an active
intro/attack before readied actors have received their release signal.

Validation executes the actual generated selection gate/phases and the
existing Valley evade function, plus production Nanyd actor/motion helpers.
Cases cover 20/10 actors, all quadrants, first-live selection, finished/null
actors, counter cycles, release gates, exact old-counter boundaries, variant
table changes, world targetable binding, RNG order, native damage arguments,
coordinated one-hit behavior, queen-loss timeout/meter retirement and the
queen-loss barrier across an active attack. Original PRG literals verify helper
EAs, tables, coefficients, masks, radii, damage flags and thresholds. The
12,000-frame motion/map check, single/horned attack checks, Valley projectile
checks, meter reward lifecycle and accepted Scatter sequence regressions pass.
Full Vita compile/link/VPK packaging passes. This is source/build verification;
Nanyd attack visuals, contact damage and continued play still need device proof.

Dual/variant formation attacks, Lathum's field interaction/formation and remaining
performance/visual acceptance remain open. The accepted passage camera is not
reopened by this change. No hardware result is inferred from these tests.

## Horn-warning intro uses shared native speech and camera

Original 0605B274..0605B2FC sets mode 7 and calls camera setup 06061F88
with the formation head, distance 50000 and zero final arguments. At the
2000 gate it calls speech 06070E0E with text 9, duration 15 and AF9F4
(B_A5_03.PCM), then restores via 060818D0. The older adapter omitted the
camera setup, used a custom PCM/text task, and restored the default camera
through a different helper. The generated single formation now uses the
existing shared native speech service and exact camera setup/restore calls.
It no longer injects its own copy of the Nanyd dialogue implementation.
The original old-counter >75 completion rule remains unchanged.

Generated intro gates, exact service arguments, all 65,536 signed timer
values and original factory/PCM/operand contracts pass. Single controller
and attack regressions and full Vita packaging pass. Actual Gash voice,
subtitle/camera timing and subsequent player control remain device checks;
no new acceptance is claimed. This supersedes the older custom single intro
implementation, while the separate accepted Nanyd dialogue path is unchanged.

## First Nanyd introduction shares native speech/camera services

Original 0605CEC0..0605CF44 uses the same camera setup and negative-flag
speech factory as the horn warning. CFE4/CFE8 contain distance 50000 and
setup helper 06061F88; CFEC names B1018 (B_A5_00.PCM); CFF0/CFF4 name
speech 06070E0E and camera restore 060818D0. Text 4, duration 15 and old
counter >84 are preserved. The generated formation now calls the existing
shared speech service and authored camera helpers instead of injecting the
custom NanydDialogue PCM/text task. Scatter and attack controllers remain
connected through their guarded update anchor.

Generated intro gates/service arguments, all signed word timers and original
PRG factories/filename/constants pass. Nanyd controller and attack regressions
and full Vita packaging pass. The earlier accepted first-swarm line remains
historical acceptance of its earlier build; this service consolidation still
needs device voice/subtitle/camera and player-control acceptance. No story
flags or battle-entry conditions were forced.

## Shared native voice tasks now reach Vita raw PCM playback

The speech service audit exposed a lower-level integration gap: native
sPCMVoiceTask calls openPCMFileForStreaming/drawPCMDebugDisplay, which still
used desktop fopen/SoLoud and elapsed desktop time. Vita raw PCM was connected
only through sCutsceneTask. The generated adapter now removes desktop raw-file
playback and routes native voice Draw through the existing raw PCM ring updater.
Disc-backed openFileForStreaming, native metadata, stream state and voice task
ownership remain intact. Battle and field voice tasks reuse the town PCM queue,
backpressure, wrap handling, drain and termination rather than another player.

Raw PCM frame progress now subtracts queued sample frames from submitted sample
frames. This prevents the speech state from observing end-of-audio merely
because all bytes were queued. Final state/frame are published after drain.
The change also applies to the existing town raw PCM user of this shared helper.

Generated voice Draw/shared updater tests cover both native sample rates, ring
wrap, packet sizes, busy retry without consumption, partial playback progress,
queue drain, voice-task finish and invalid payload rejection. Shared single
speech voice/timer/skip/chaining regressions and full Vita packaging pass.
This verifies the service boundary, not audible output, disc reading in a live
run, cancellation timing or device performance. Gash/Lathum/field speech and
town raw PCM need listening/timing acceptance on hardware.
