# Combat rewards and progression audit

Goal 2 is active. Current combat performance and loading behavior are accepted;
do not equate that with reward/progression acceptance.

## Rank identity correction

Original US Disc 1 BTL_A3.PRG is at LBA 258144, length 385044, base 06054000.
At 0606EA60, MOV.W @(2,R0) reads byte offset 4 from the manager, not offset 2:
the SH2 word displacement is scaled by two. This is m4_previousBattleOverlayId.
At 0606EA88, MOV.W @(4,R0) reads offset 8, m8_previousSubBattleId. The table
literal at 0606EB60 is 060AB010; the manager getter at 0606EB5C is 0600675E.
The existing rank adapter incorrectly indexed with current/remapped IDs.

The shared adapter now uses the previous IDs as the native code does. No
thresholds, scoring bands or XP multipliers change. battle_rating_test.ps1
checks these original disc instructions/literals and executes the production
include. The fixture deliberately makes current IDs 99/99, different from
the previous IDs, while checking all 25 score combinations over 17 overlays
and ten subbattles, threshold equality, update division and null tables.
This is a bounded source/instruction check, not whole-battle Vita acceptance.

## Native bonus-item selection and award

The guarded generated adapter replaces only the result-screen award stub.
Original 0606ECA2 selects the table at 060AAFD8 using previous overlay/subbattle
IDs. Rows are six bytes: two item words, then two chance bytes. A nonnull
table consumes one modulo-100 native RNG roll even for a low rank. Great Fight
uses the first item/chance; Excellent uses the second. Equality fails. Lower
ranks do not choose a bonus, and a null table consumes no RNG. A previously
recorded m480[0] is preserved if the roll does not choose a new bonus.

Native helper 0605B302 writes reward slot m480[0] via 0605B2CC; it does not
increment inventory there. The result routine then displays that reward and
updates inventory, with a 99 consumable cap or an ownership bit at F3+item
for IDs >=4D. This uses the existing game-state methods, not a separate reward
inventory. Native label config 060ABA00 and name/icon config 060ABAB0 drive
the announcement. The icon/name services read the existing COMMON item table.
The name is at config X+2; palette is restored afterward. The native debug-only
table dump controlled by battle debug flag 17 is not implemented here.

The first eight A3 rows from the disc are Macral Liquid (1C) for Great Fight,
Macral Shell (1D) for Excellent, both chance 30. These are read from the disc,
not installed as per-enemy constants in the adapter. Later rows retain their
authored values. A successful rank does not guarantee a bonus.

battle_rewards_test checks original instructions/literals and feeds those
eight rows to the actual production include for all five ranks and all 100
rolls, deliberately using different current/previous IDs. It additionally
tests asymmetric thresholds, equality, 0/100 chance, negative reward, null
table, pre-recorded reward, actual common.h inventory methods, 98->99 and
99->99, flag-backed items, and native-layout label/icon/name publication.
RNG/UI/disc-memory services are mocked; this is not whole-Saturn execution.
Existing growth/migration and corrected rank tests pass. Both Vita build
configurations pass, with the battle configuration compiling the award adapter.

Hardware gate: normal victories should sometimes show/award these items when
rank and roll qualify. Inspect [BattleRewardRoll] and [BattleReward], check the
inventory, then save/reload and check again. Do not force RNG or alter ranks to
obtain an award. Goal 2 is not complete: enemy-result count/best-rank records,
earned Berserk eligibility/unlocks/announcements, and hardware persistence
validation remain. The existing XP/growth path was not replaced by this work.

## Saved enemy-record bookkeeping

Original rank computation does not return immediately after choosing a rank:
0606EBC8..0606EC8C walks ten engine m484 entries, stops at the first negative
entry, and updates byte pairs at 0024BCA0. Count saturates at 255; best rank
only increases. Null rank tables still take this bookkeeping tail with rank
zero. The first implementation omitted that tail.

The adapter now updates the existing battleResults array (104 hex bytes,
130 pairs), using the existing native load/save ownership, not new storage.
The life-meter factory already records unique encountered type IDs in m484;
these counts are per encountered type per result, not one increment for each
individual killed actor. The native ten-slot/sentinel traversal is retained.
The debug-only rank print remains outside this implementation.

Tests execute the production rank+tail with empty/sparse/full lists, first
negative sentinel, highest valid record ID, all five rank values, 255 cap,
no best-rank downgrade, and null-table behavior. Original tail constants/
instructions and saved-array load/store assignments are checked separately.
Those source checks establish ownership, not durable on-device roundtrip.
Hardware save/reload and earned-Berserk work remain open.

## Earned Berserk audit (in progress)

Native 0606F308 scans class tables from 060AC094 in descending class order
3..0, stopping each table at a zero item ID (word +6 of each eight-byte row).
Requirements are bytes +1..+4 compared with the four saved proficiency levels
+1; byte +0 is not checked by this routine. Already owned abilities are skipped.
Unlocks set the existing inventory ownership bit, then enqueue the item in the
result's class-specific list and increment that class's count. No extra RNG.
0606F270 presents up to four queued entries in ascending class order.

The growth adapter now preserves pre-update proficiency levels, as native
0606F002 does at result +75+i. Host tests cover both advancing and unchanged
levels. This is result-local state, not a new save format. The unlock scanner
and learned-ability presentation are not wired yet; this change alone does not
claim that earned Berserks work. Vita reward/save-reload acceptance is pending.
