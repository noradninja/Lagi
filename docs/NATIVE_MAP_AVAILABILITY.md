# Native Map availability

Live integration update: `lagiAzelResolveMapParentSource` and `createMapTask`
now use the native parent when the availability result is provable. Story
bit193 unset bypasses the field-route test; an authored zero route yields zero
for either value of0024B89C. A nonzero route still returns unresolved, never
a forced0/1. Town availability uses its independent established branch. An
enabled world route additionally requires the actual module-manager overlay
entry5 to be non-null; the current trimmed build leaves it null and retains
the placeholder rather than altering availability. See
`NATIVE_MAP_PARENT_TASK.md` for supported cases and remaining limitations.

Read-only Disc 1 MAIN06016D88..06016E62 establishes the selector rules:

| Condition | Field slot | World slot | Initial selection |
| --- | --- | --- | --- |
| Default / story bit193 unset | 0 | -1 (absent, native cover) | Field |
| Story bit193 set, no route | 0 | 0 (selectable, confirm rejected) | Field |
| Story bit193 set, route enabled | 0 | 1 | World |
| Native field context exists | 1 | unchanged | Field (overrides World) |

An enabled route is scene class0 with a nonzero0601243A result, or scene
class2 with the signed current-town byte0604A2E0 different from060300CC(0).
Other scene classes do not enable the route. The original field-context
resolver runs last. Field-map inventory ownership is not an availability input.

`lagiAzelMapAvailabilityFromNative` now models these decisions, without
rendering or manufacturing live state. Existing services can consume the two
slot values to select authored messages and native input decisions.

## Field route table

MAIN0601243A..0601249A first reads byte0024B89C; zero returns no route.
With that gate nonzero, field1 additionally requires story bit193 and reads
COMMON20A3E4[subfield] as a big-endian long. Other fields read the pointer at
COMMON20A4D4[field], return zero for a null pointer, otherwise read its
subfield long. The return value is retained, not normalized to a Boolean.

`lagiAzelReadFieldWorldMapRoute` implements this lookup with checked COMMON
spans, aligned table addresses and 64-bit offset arithmetic. It does not infer
a subfield count from adjacent tables. False indicates malformed input; a
successful zero is a legitimate native no-route result. Bounds guards are
port safety checks, not additional authored progression rules.

## Unresolved live binding

The meaning and live owner of0024B89C are **not established**. The route
adapter still requires its value explicitly. Live source resolution only
bypasses that unknown when both gate values yield the same native result.
Do not replace it with map ownership, fieldTaskVar3, a guessed progress bit,
or a forced true/false value. The town branch is now bound separately by
`lagiAzelResolveTownMapAvailability`: native06030060 writes npcData0.m5C at
0604A284+5C, and060300CC(0) reads signed COMMON2166E4[0]. The production
service passes65536 signed-byte comparisons plus native story/scene and
malformed-COMMON guards, and Vita syntax. It is now connected to the parent
source resolver, but does not resolve the separate field gate. World travel
remains unavailable while its actual overlay-table entry is null.

The earlier availability-only checkpoint made no visible change. The later
partial live parent integration is not a claim of complete Map-menu coverage.

## Checks

### Whole-disc literal-owner investigation (2026-10-16)

The read-only `tools/map_gate_literal_scan.cpp` scans every user-data byte
of the663,518,016-byte Disc1 MODE1/2352 image for nearby low-RAM literals,
including SH2 address aliases. The sole exact0024B89C hit is LBA82,
user offset530hex: MAIN offsetC530hex at native06012530, the literal
loaded by the already traced availability read06012440. No second direct
literal owner was found anywhere in this image. This does **not** prove the
flag is constant or uninitialized: an indirect/computed writer or external
resident initialization may exist. Compressed resources, cross-sector words
and runtime-generated addresses are not resolved by this scan. Nearby and
unaligned hits must not be mistaken for executable cross-references.

This narrows the next trace to indirect ownership/initialization rather than
repeating exact-pointer searches; it does not authorize forcing the gate on.
The active Map parent remains incomplete. Its field-view child, selector and
confirmation services are not yet a complete working Map-menu integration.

PowerShell after entering the VS developer environment described below:

```powershell
cl /nologo /O2 /EHsc tools/map_gate_literal_scan.cpp /Fe:../map_gate_literal_scan.exe /Fo:../map_gate_literal_scan.obj
& ../map_gate_literal_scan.exe 'D:\Users\Noradninja\Desktop\Panzer Dragoon Saga Disc 1 of 4 (U)(Saturn)\042 Panzer Dragoon Saga Disc 1 of 4 (U).bin'
```

The host test executes the production inline logic over432 combinations of
story bit, scene class, route, signed town comparison and field context. It
checks route short-circuiting, null tables, native longs and malformed spans,
indices and alignment. With the original Disc 1 image it also verifies the
field1 direct table, field2 shared table (first four routes1, fifth0), and
authored null field entries, without modifying the disc.

PowerShell, from the checkout:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
New-Item -ItemType Directory -Force ../map-availability-test | Out-Null
cl /nologo /std:c++20 /EHsc /Iinclude tests/map_availability_test.cpp /Fe:../map-availability-test/test.exe /Fo:../map-availability-test/
& ../map-availability-test/test.exe 'D:\Users\Noradninja\Desktop\Panzer Dragoon Saga Disc 1 of 4 (U)(Saturn)\042 Panzer Dragoon Saga Disc 1 of 4 (U).bin'
```

The adapter/header Vita syntax and prior cursor/selection/lifecycle host
regressions also pass. No full VPK link or Vita availability/menu validation
has run for this checkpoint.
