# Native Map availability

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

The meaning and live owner of0024B89C are **not established**. The current
adapter requires its value explicitly and is not connected to the Map parent.
Do not replace it with map ownership, fieldTaskVar3, a guessed progress bit,
or a forced true/false value. The town branch is now bound separately by
`lagiAzelResolveTownMapAvailability`: native06030060 writes npcData0.m5C at
0604A284+5C, and060300CC(0) reads signed COMMON2166E4[0]. The production
service passes65536 signed-byte comparisons plus native story/scene and
malformed-COMMON guards, and Vita syntax. It remains unconnected to the
parent, and does not resolve the separate field gate. Native unavailable-slot covers
and world-map confirmation/transition still require integration.

No visible Map-menu change or complete viewer is claimed by this checkpoint.

## Checks

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
