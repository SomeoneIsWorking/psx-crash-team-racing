# 0040 — A disc read holds one present for 200 ms

Status: fixed (2026-10-28). Owner: `game/disc/async_disc_owner.cpp` (`DiscReadOwner::deliverIfDue`).

## Observed
Every all-title 40 s paced hidden run reported one present interval of 180-200 ms in the attract loop. Unpaced
(`scratch/player-bugs-3/r1`, `PSXPORT_DEBUG=presentseq`, 1600 fences) the same field, fence 1435 to 1436, took
202 ms of host time, so it is the field's cost, not pacing.

## Cause
The field issues a 243-sector `BIGFILE.BIG` read (LBA 52753) through `FUN_800321b4` with completion callback
`0x80032110`, then polls for that callback inside the same field. `deliverIfDue` held the callback until the
drive's read time had passed in emulated CPU time: 243 sectors at double speed, about 1.6 s of guest time. The guest
spun through that whole wait in one field (17 polls 308k ticks apart, `scratch/player-bugs-3/r3`), costing 200 ms of
host time. Disc reads are instant by design; the drive time was only there so the loader publishes its state
before the callback runs.

## Fix
The completion is owed until the loader that called `CdRead` has returned: `noteTransferComplete` records the
guest stack pointer at the `CdRead` call, and the next budget exit with the stack above it delivers the callback.
The drive-time wait is gone.

## What the loader publishes
Every read that carries a completion callback is issued by the load-queue pump `FUN_80032dc0`: it calls
`FUN_800321b4` (type 1) or `FUN_80031e00`/`FUN_80031fdc` (types 2 and 3, which call `FUN_800321b4`), stores the
return into the current entry's pointer word `0x80083A48` and then decrements the queue count `gp+0x136`. The
callback `FUN_80032110` hands `&0x80083A3C` (that entry) to the entry's own callback (`gp+0x8F0`, `FUN_80032d8c` for
type 1). So "published" is the store to `0x80083A48` after the loader returns. The stack test stands for it because
that store is the pump's next instruction after the call; it would deliver one block early if a budget exit fell
between the return and the store, and it falls back to the field boundary if a loader polled inside its own frame.
Neither has been seen. Comparing `0x80083A48` against the returned pointer would be exact for type 1 only, since
types 2 and 3 store their own return values. (Ghidra pipeline audit flagged its pre-script record for this CTR
project; the bodies above match the disassembly at the cited call sites.)

## Shown
Same run after the fix (`scratch/player-bugs-3/r4`, 3000 fences unpaced): no field longer than 30 ms after boot
(the remaining 59 ms at fence 446 is the renderer start, not a read); callback 30 is still delivered in the same
field 1435, so the attract loop's timing is unchanged, and the next present shows the LOAD screen
(`scratch/player-bugs-3/evidence/0040_f1437.png`).
