# Berserks: Disc 1 data and native support

This is our reference for bringing Berserk attacks up on Vita. The game remains in charge of the attack; Lagi supplies the missing runtime services, and Neptune renders the commands the game produces.

**Source:** US Disc 1 `BTL_A3.PRG` and `COMMON.DAT`, cross-checked against the pinned Azel checkout (`extern/Azel`, `4cebb24`) and the current Lagi recovery code. Checked 20 November 2026. This is a data and source audit, not a claim that every attack works on Vita.

## What the Disc 1 table contains

`BTL_A3.PRG` contains the proficiency unlock table used by the battle-results logic. It is a **full progression table** embedded on Disc 1, **not a list of techniques that a normal playthrough can obtain before the disc change**. The higher ranks can require substantial leveling, and ordinary story access is a separate question.

At Saturn address `060AC094` there are four big-endian pointers to eight-byte unlock records. The table pointers are `060ABF74`, `060ABFA4`, `060ABFD4`, and `060AC004`. Each record stores four class requirements in bytes +1 through +4 and the item ID at +6. An item ID of zero ends the table. The original results logic compares each requirement with the corresponding stored proficiency level **plus one** and skips abilities already in inventory. Actual names below come from `COMMON.DAT`, not Azel enum identifiers.

| Laser proficiency | Required rank | Item ID |
| --- | ---: | --- |
| Chain Laser | 3 | `0xA0` |
| Laser Storm | 13 | `0x9F` |
| Prism Laser | 20 | `0x99` |
| Lightning Storm | 28 | `0x9C` |
| Berserker Rage | 40 | `0x9E` |

| Spiritual proficiency | Required rank | Item ID |
| --- | ---: | --- |
| Holy Sphere | 5 | `0xA1` |
| Cleansing Wave | 13 | `0x97` |
| Judgement Day | 20 | `0x95` |
| Armageddon | 28 | `0x96` |
| Dragon Phoenix | 40 | `0x98` |

| Defense proficiency | Required rank | Item ID |
| --- | ---: | --- |
| Heal | 2 | `0xAA` |
| Vengeance Orbs | 13 | `0x94` |
| Heal-Maxis | 20 | `0xAB` |
| Shield | 28 | `0xA5` |
| Genesis | 40 | `0xAC` |

| Agility proficiency | Required rank | Item ID |
| --- | ---: | --- |
| Phantom Slashers | 4 | `0x91` |
| Escape | 13 | `0xB1` |
| Wraith Slashers | 20 | `0x92` |
| Recover | 28 | `0xAD` |
| Onslaught | 40 | `0x93` |

These 20 records are the exact proficiency table on Disc 1. Rank requirements are **class ranks**, not Edge's overall level. The four growth counters use battle XP weighted by the dragon's class balance; see `src/integration/lagi_battle_growth.inc`.

## Techniques outside the proficiency table

The same Disc 1 `COMMON.DAT` names the following Wing techniques:

| Technique | Item ID |
| --- | --- |
| Assault Wing | `0x9A` |
| Berserker Wing | `0xAE` |
| Protection Wing | `0xAF` |
| Swift Wing | `0xB0` |
| Healing Wing | `0xB2` |

These do not appear in the 20-row proficiency table. They are associated with the dragon's Wing abilities and are separate from ordinary level-up awards. The Valiant Wing progression route is the relevant Disc 1 gameplay checkpoint, but the exact grant/availability conditions still need an original event trace before we treat them as verified by disc scripts.

Other named attacks in `COMMON.DAT` outside the proficiency table are Plasma Vortex (`0x9B`), Plasma Swarm (`0x9D`), Hunting Scythe (`0xA2`), Astral Phantoms (`0xA3`) and Energy Prism (`0xA4`). Energy Prism is associated with the Valley waterfall pickup; Astral Phantoms has been reported as an early camp/level unlock. Those acquisition paths are not established by the proficiency table itself and remain to be checked against Disc 1 event scripts. In particular, an attack name being present in Disc 1's generic battle data is not proof that it is normally obtainable on Disc 1.

## Item IDs must take precedence over enum labels

The pinned Azel `items.h` still has labels that do not agree with the text in `COMMON.DAT`. For example:

| ID | Original `COMMON.DAT` name | Azel enum identifier |
| --- | --- | --- |
| `0xAA` | Heal | `mAA_healMaxis` |
| `0xAB` | Heal-Maxis | `mAB_genesis` |
| `0xAC` | Genesis | `mAC_berserkerWing` |
| `0xB1` | Escape | `mB1_healingWing` |
| `0xB2` | Healing Wing | `mB2_fieldMap` |

This is a **naming inconsistency in the reconstruction**, not yet evidence of a broken runtime dispatch. Follow the original numeric IDs and item records when auditing ability selection, cost, category, animation, and effect. We should not quietly correct the behavior by relabeling an effect in Lagi.

## Native execution path

`battleEngine_useBerserkApplyEffect` (`BTL_A3 06066C54`) selects and starts original Berserk producers. The engine handles startup, BP and combo consumption, state transitions, and eventual camera restoration. Child tasks handle their own motion, hit timing, drawing, and cleanup. The attack is not complete just because its factory has been registered or damage has been computed.

The missing functionality is a mix of native producer logic and shared services. The same families repeat across multiple attacks:

| Shared capability | Useful test cases | Current gap |
| --- | --- | --- |
| VDP1 beam/trail strips, history and palette relocation | Chain/Prism Laser, Lightning Storm, Energy Prism | Shared trail drawing exists; several native trail update/draw paths remain incomplete |
| Homing model/beam tasks | Laser Storm, Lightning Storm | Homing-laser initialization and drawing routines are not fully restored |
| Disc-backed animated models and hotpoints | Phantom Slashers, Astral Phantoms, Dragon Phoenix | Some producers never create their original model or child task |
| Particle emitters and animated quads | Plasma Swarm/Vortex, Dragon Phoenix, Laser Storm | Several emitter constructors and updates are unfinished |
| Targets, damage, hit events and completion | Phantom Slashers, Astral Phantoms, Laser Storm | Some loops and attack-finished handoffs remain incomplete |
| Camera, VDP2 fades, lighting, audio and ownership | All families | Each producer needs verification across its full lifetime |

The immediate shared-service investigation should pair **Lightning Storm and Laser Storm** because both refer to the original homing-laser/trail machinery. **Chain Laser, Phantom Slashers, Holy Sphere and Energy Prism** make useful Disc 1 functional tests. This is a proposed test order, not a change to original unlock rules.

The implementation boundary stays the same: **Azel decides. Lagi services. Neptune renders.** We restore original producer behavior where it is missing, provide the reusable runtime operations it expects, and extend Neptune only when an actual emitted command exposes a renderer gap. No Vita-specific substitute attacks and no per-attack rendering shortcuts.

## Evidence and remaining checks

- `src/integration/lagi_battle_berserk_rewards.inc`: unlock table traversal and inventory awards.
- `src/integration/lagi_battle_growth.inc`: four-class cumulative proficiency XP and thresholds.
- `extern/Azel/AzelLib/battle/battleEngine.cpp`: dispatch and BP/state logic.
- `extern/Azel/AzelLib/battle/berserk_*.cpp`: original producer reconstruction and visible gaps.
- `docs/BATTLE_EFFECTS_RECOVERY.md`: effect recovery evidence and known omissions.
- `BTL_A3.PRG`, `COMMON.DAT`: definitive byte-level unlock tables and display names.

Still needed: trace the non-proficiency acquisition events, verify the original instruction ranges behind unfinished producers, and capture individual attacks on Vita for timing, visuals, damage, and cleanup. Compilation and host tests alone do not establish hardware parity.
