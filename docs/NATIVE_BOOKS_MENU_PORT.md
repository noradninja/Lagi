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
the caller still needs the native logical heading-count validation before
selection. This reader does not implement body text/images or a Books task.

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

Reconstruct helpers `06016004`, `0601604E`, `06016088`, `06016194`,
`06016328`, `06016396`, `060164AC`, `060164EC`, `0601653E`, `060165D4`,
`06016738`, the cursor task, and inventory return `060183D8`. Preserve the
original data-driven book text/images, ownership and fades. Vita acceptance
must cover Books entry, owned/unowned items, pages/images, cancel/reentry,
key-item launch and inventory return without lost fonts or cursor tasks.
