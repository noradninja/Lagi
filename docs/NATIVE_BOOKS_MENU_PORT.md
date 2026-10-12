# Native Books/key-item screen port map

Status: original disc research, not an implemented screen or hardware result.
Keep `createMenuBKTask` disabled until its real constructor and renderer flow
are present. Inventory category 1 remains unfinished.

Implemented prerequisite: `lagi_azel_books_menu.cpp` now provides the native
`06016004`/`0601604E` graphics backup/restore boundary. It allocates `0x169A0`
bytes with `dramAllocate`, copies VDP1 `0x12000/0x580`, VDP2
`0x20000/0x16000`, and CRAM `0x7E0/0x420` in native order, then restores,
releases and invalidates renderer caches. The translation unit passes Vita
C++ syntax checking. It is not yet invoked because the Books task is still
unported; no screen or runtime ownership roundtrip is claimed validated.

The same module now implements `06016ADA` item-to-table-index lookup and
`06016328`/`06016396` owned-index list construction/initial selection. It reads
COMMON `0020DDC8` and native ownership bits `0xF3+item`, stores compact table
indices rather than item IDs, and defaults selection to zero for ordinary
entry or an unmatched requested index. Empty ownership remains an empty list;
the eventual task must not dereference its selected position then. A 576-case
selection model/source guard and actual helper Vita syntax compilation pass.
These helpers are not connected to an implemented screen yet.

Heading resource reader now follows `060164AC`, `060164EC`, `06016582` and
`06016598`: first word is the book-record offset, each book record is 16
bytes (title offset +0, page-table offset +4), each heading record is eight
bytes (heading offset +0, first body page +4, body page count +6). All offsets
are relative to the loaded resource base and big-endian. Returned strings
borrow its lifetime. Range/terminator checks prevent out-of-image pointers;
the reader also rejects selections outside the native heading count at book
record +12. This reader does not implement body text/images or a Books task.

`tests/books_resource_test.cpp` executes the production portable reader against
the original US Disc 1 resource directly, opening the image read-only. All 22
books and 61 headings pass, as do invalid book/heading and truncated/null input
rejection checks. This is host execution, not Vita screen validation. The
resource is 34,988 bytes, with its book table at `0x874C`: the eventual loader
must allocate the actual file size, not the reconstructed `0x7E80` allocation
(32,384 bytes), which cannot hold this disc's resource.

The body-page reader follows `060165D4`: book +8 points to the BE32 body
table, and book +14 holds the logical page count. Values greater than 3
are text offsets; 1 and 2 select native illustration setup, while 0 and 3
disable it. The reader preserves these commands rather than treating them
as strings. Host execution covers all 168 real body pages, including two
illustration commands. It does not draw them or implement task navigation.

Native `06016738` first handles cancel (new-input bit 1), then selects an
input focus in `[0,2]` using `06015DA4`: owned book, heading, or body page.
`06015D30` changes the focused index within its logical count. Changing a
book resets the heading to zero; changing a book/heading sets the global
body page to that heading's first page. Changing the body page calls
`060166C2`, which scans heading ranges and selects the containing heading,
defaulting to zero if none matches. The portable helper now implements
that final association; host execution checks all 168 real pages. The
input helpers, cursor flags, fade transition and task remain unconnected.

Evidence: read-only inspection of the user-provided US Disc 1 MODE1/2352
image. `1ST_READ.PRG` loads at `06006000`; COMMON.DAT at `00200000`.
No game files or `extern/Azel` were modified or copied into the repository.

## Entry and selection

- Books constructor: `06016AC8`, task definition COMMON `0020E3C4`, size
  `0x2B0`, argument `-1`.
- Inventory key-item constructor: `06016A7C`, same definition and size,
  argument is the key-item lookup index.
- Task definition: Init `06016850`, no Update, Draw `060168DA`, Delete
  `06016A42`.
- Lookup `06016ADA` scans exactly 22 signed 16-bit entries at COMMON
  `0020DDC8`, returns table index or `-1`.
- Native item ordering:
  `90,80,81,82,6F,70,86,69,77,78,79,7F,8A,8E,8F,7E,7A,7B,7C,7D,8D,8B`.
  Values are hexadecimal item IDs, not invented labels.

## Resources and ownership

Init backs up `0x240` bytes from `0604B244` to task `+0x48`, and `0x28`
bytes from `0604FD60` to task `+0x288`. It calls setup `06016004`, records
the result at `+0x44`, allocates `0x7E80` bytes (pointer `+0x10`), loads
`MENUBK.BIN` (filename `06043248`), and computes its internal data pointer
as buffer plus the first big-endian word (`+0x14`). It calls `06016194`
before loading `MENUBK.FNT` (filename `06043254`, font ID `+0xC`).
Cursor task definition is COMMON `0020E3B4`, constructed with size `0x14`.
Init invokes selection helpers `06016328` and `06016396`, preserving the
caller's vsync count at task `+0`, then setting vsync count to one.

Delete `06016A42` unloads font ID `+0xC` through `060250E6`, restores vsync,
releases the data buffer, invokes `0601604E` with `+0x44`, then restores the
two backed-up state blocks. These are required lifecycle operations, not an
optional visual approximation.

## Draw state machine

Draw dispatches task state `+0x4` from 0 through 8. The helper relationships
below are verified call sites; helpers still require reconstruction.

| State | Native calls and transition |
| --- | --- |
| 0 | `06016738`; then `06016088(1,1)` |
| 1 | Sound 1; set countdown 8; advance |
| 2 | Decrement countdown; call `06016088(countdown,8)`; advance at zero |
| 3 | Set/clear text region `(0,0x22,0x2C,0x1C)`; call `06015AF0`; set two fade channels to zero; advance |
| 4 | Marks this task finished |
| 5 | Set countdown 8; advance |
| 6 | Decrement countdown; call `06016088(countdown,8)`; advance at zero |
| 7 | Call `060164AC`, `060164EC`, `0601653E`, `060165D4`; set counter zero; advance |
| 8 | Set two fade channels to `0xC210`; increment counter; call `06016088(counter,8)`; return to state 0 at eight |

## Inventory transition

On a successful lookup, `06018612` stores the index at inventory task
`+0x18` and calls `06018388`: disable two presentation channels, finish the
two cursor children, unload `ITEM.FNT`, then enter inventory state 4.
State 4 advances; state 5 constructs the Books/key-item child and retains
its pointer at `+0x14`; state 6 waits for it to finish. Return calls
`060183D8`, redraws inventory rows (`06018298`) and the selected details
(`06017F7A`). The port needs the complete return routine, including resource
and cursor reinitialization, before enabling key-item dispatch.

## Remaining implementation and acceptance

`06016088` window calculation is now preserved in
lagiAzelBooksCalculateWindow. Center is (176-scrollX,424-scrollY), half extents
are truncating signed multiply/divides 136*step/duration and 56*step/duration.
Both native register banks receive window-1 X endpoints doubled with inclusive
right/bottom ends. The emitted solid quad uses native asymmetric X/Y offsets,
not a guessed centered rectangle. Host checks cover the nine native reveal
steps and invalid denominator/range rejection. Register writes, solid draw,
window clipping and task integration remain to be connected; no Books screen
or hardware reveal validation is claimed.

The native service lagiAzelBooksPresentWindow now writes those window-1 fields
to both existing VDP2 register banks and submits the native solid quad through
drawGaugeVdp1 (original 0602CB5E), mode C0, RGB555 D294, depth zero. This service
is not invoked until the Books task is connected. Renderer window clipping and
generic menu solid-polygon support still need verification/integration before
the service can establish a working page; radar-only solid support is not enough.

Implemented support now includes graphics backup/restore, owned-list selection,
validated MENUBK heading/body readers and body-to-heading association. Native
06015D30/06015DA4 index input is available as lagiAzelBooksStepIndex: repeat masks
40/80 or 10/20, separate axis boundary-sound latches, movement sound 0A and
one boundary sound 05 until release/movement. Both-direction handling preserves
the original decremented candidate if decrement underflows. Disassembly was
rechecked against the original executable; 16,192 host cases exercise the
production helper. Host models and Vita syntax checks do not prove page input
on hardware. The helper is not yet connected to a Books task: do not enable
Books merely because these support routines pass tests.

Finish task integration and presentation helpers `06016088`, `06016194`,
`060164AC`, `060164EC`, `0601653E`, `060165D4`,
`06016738`, the cursor task, and inventory return `060183D8`. Preserve the
original data-driven book text/images, ownership and fades. Vita acceptance
must cover Books entry, owned/unowned items, pages/images, cancel/reentry,
key-item launch and inventory return without lost fonts or cursor tasks.

## Coupled input restoration (2026-10-11)

`lagiAzelBooksNavigate` now preserves the complete `06016738` input relationship:
cancel wins before all input, vertical repeat selects focus 0..2, horizontal
repeat operates on owned-list position/heading/global body page, a book change
resets heading zero, book/heading changes select that heading's first body page,
and body changes recover the containing heading. Accepted horizontal changes
request native reveal state 5. Cursor decrease/increase flags retain the native
pre-change logical maximum and empty lists return before cursor updates.
The task will map these results to its state and cursor child; this helper does
not invent a substitute page or enable the unfinished constructor.

Original instructions were rechecked at `06016738..0601683A` in the read-only
US executable. Host execution of the production helper passes coupled book,
heading and body changes, simultaneous axis movement, cancel priority, empty
ownership and truncated-resource rejection, alongside 16,192 index/latch cases.
Vita C++ syntax passes. These are not on-device page/task validation. Remaining
work still includes resource setup, text/illustration presentation, cursor task,
window clipping, full task lifecycle and inventory font/cursor restoration.

Text presentation support now implements `060164AC`, `060164EC`, `0601653E`
and the text-region part of `060165D4` as `lagiAzelBooksPresentText`.
It validates borrowed heading/body data before drawing, clears native regions
`(7,36,30,2)`, `(7,39,30,2)`, `(7,42,6,2)` and `(5,46,34,14)`, and uses
the existing VDP2 font services. Counter values are the one-based page within
the current heading and that heading's page count. The original separator
at MAIN `06043244` was read directly as `80 82 00 00`; the encoded glyph is
retained rather than replacing it with an ASCII slash. Commands 0..3 still
clear the body text region and are returned to the owning task for native
illustration/register handling; this service does not implement those graphics
changes. Its Vita syntax passes, but it is not connected to a Books task and
neither font appearance nor hardware text-region behavior is validated.

The subsequent illustration switch completes the register/scroll portion of
`060165D4`: values 1/2 call native `setVdp2LayerScroll(2,-104,256)` /
`setVdp2LayerScroll(2,152,256)` and `scrollMenu` (`06015B08`), then set
NBG2 priority to 5. Text and commands 0/3 clear that priority. All paths retain
the other `PRINB` bits and mark the pending VDP2 bank dirty, rather than changing
both banks or disabling other layers. The body text region is cleared first,
as in the executable; text is then drawn only for values greater than 3.
This supersedes the previous paragraph's unimplemented-switch limitation, but
does not load either illustration or implement the task's graphics setup.

`tests/books_presentation_test.ps1` mechanically extracts the actual production
service and executes it against production resource readers with mocked VDP2
calls. It checks all four command values and text, exact four native regions,
one-based heading counter, encoded separator, scroll/publication calls, priority
preservation and rejection without drawing for a truncated resource. Host
execution and Vita helper syntax pass. Native image/font rendering and task
ownership remain unverified; the Books constructor is still disabled.

## Native graphics-setup window prerequisite

Read-only MAIN inspection of `06016194` and data `06043178..0604322B`
confirms three setup tables. The NBG1 table selects CHCN=1, CHSZ=1, PNB=1,
CNSM=0, SCN=4, PLSZ=0 and CAOS=3. NBG2 has the same format with CAOS=5,
plus window-1 enable/action commands 35=1 and 38=1. NBG3's table is
33=1, 34=1, 37=0, 35=1, 38=1: combine windows, enable window 0 inside and
window 1 outside. This is not a sprite WCTLD table. The original routine at
`06033864` is NBG3 setup, and its dispatch table `060338C0` maps these commands
to `06033A1A`, `06033A34`, `06033A7C`, `06033A4A` and `06033A90`.

The reconstructed setupNBG3 omitted all window commands and would assert when
executing this native setup. The generated VDP2 adapter now restores commands
33..39 against the high byte of WCTLB: shifts 15/9/11/13/8/10/12. This preserves
NBG2's low byte and unrelated NBG3 window bits; it uses the pending bank and
existing dirty handoff. Upstream files remain unchanged. The shared production
bit helper passes 917,504 exhaustive prior-register / command / binary-value
cases and the actual MENUBK sequence on host. Actual generated VDP2 and helper
Vita syntax pass. Renderer clipping is still not connected to Books, and no
hardware window/page validation is claimed.

Graphics loader evidence also establishes original filenames MENUBK2.BIN at
`0604322C` and MENUBK.CGB at `06043238`, VDP1 destination 25C12000 and
COMMON palette 0020DDF4 -> CRAM 07E0 for 0420 bytes. Retain these native
resource boundaries when completing setup; do not replace the illustrations
with generated images or import disc resources into the repository.

## Graphics setup service

`lagiAzelBooksSetupGraphics` now restores `06016194`: native cycle patterns,
the three verified setup tables, NBG1/2 maps at 20000, priorities 0400/0705
and four sprite priority words 0606, MENUBK2.BIN / MENUBK.CGB loading, COMMON
palette copy, layers 1/2/3 scroll, scroll publication/reset, window 0 and the
collapsed initial window 1, then full native text-region clearing. Window-0
bottom retains the executable's un-decremented endpoint, unlike its X end.
TVSTAT-dependent vertical offset remains owned by setVdp2LayerScroll rather
than guessed by this service. File sizes are checked against the exact native
backup spans before writes. Load failure returns false; the eventual task must
restore its retained graphics/register backups before returning to inventory.

NBG2's missing window-1 commands are also restored through the generated VDP2
adapter. Its low-byte window controls and NBG3's high-byte controls each pass
917,504 exhaustive production-helper bit preservation cases. Actual graphics
setup host execution uses mocked platform services and validates the three
tables, map/resource destinations, palette, priorities, scroll/window/clear
calls, pre-write missing/oversize rejection and load-failure return. Vita helper
syntax passes. This service remains uninvoked: Books task resource/font loading,
backup ownership, cursor, fades, window clipping and inventory return are still
required before the constructor can be enabled. No screen/device success is
claimed from these tests.
